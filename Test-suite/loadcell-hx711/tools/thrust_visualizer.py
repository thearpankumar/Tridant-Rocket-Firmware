#!/usr/bin/env python3
"""
Thrust Test Data Visualizer
============================
Interactive web dashboard for visualizing thrust test data from ESP32/Teensy UART logs.

Usage:
    python thrust_visualizer.py --file <path_to_data.txt>
    python thrust_visualizer.py  # Opens with file upload interface

Features:
    - Interactive graph with zoom, pan, and hover
    - Statistics panel (min, max, mean, std dev, sample rate)
    - Data table view with all measurements
    - Filter valid/invalid data points
    - Export to PNG, HTML, and CSV
"""

import argparse
import re
import io
import base64
from pathlib import Path
from datetime import datetime

import pandas as pd
import numpy as np
from dash import Dash, html, dcc, dash_table, callback, Output, Input, State, no_update
import plotly.graph_objects as go


# =============================================================================
# Data Parsing
# =============================================================================

def parse_thrust_data(content: str) -> pd.DataFrame:
    """
    Parse thrust test log data from UART output.

    Expected format per line:
    <local_timestamp>,<VALID|INVALID>,$thrust_test,<msg_id>,DATA,THST,<value>,N,<esp_timestamp>*<checksum>

    Returns:
        DataFrame with columns: local_ts, valid, device, msg_id, data_type,
                               sensor, value, unit, esp_ts, checksum
    """
    records = []

    # Pattern to match valid data lines
    # Example: 3890,VALID,$thrust_test,0000,DATA,THST,0.000,N,0*0E
    # Note: value can be negative (e.g., -105.497)
    pattern = re.compile(
        r'^(\d+),(VALID|INVALID),\$(\w+),(\d+),(\w+),(\w+),(-?[\d.]+),(\w+),(\d+)\*([0-9A-Fa-f]{1,2})$'
    )

    # Pattern to extract embedded data from corrupted/concatenated lines
    # Matches: $thrust_test,<msg_id>,DATA,THST,<value>,N,<esp_ts>*<checksum>
    embedded_pattern = re.compile(
        r'\$(\w+),(\d+),(\w+),(\w+),(-?[\d.]+),(\w+),(\d+)\*([0-9A-Fa-f]{1,2})'
    )

    for line in content.strip().split('\n'):
        line = line.strip()
        if not line or not line[0].isdigit():
            # Skip empty lines and header/metadata lines (e.g., "=== UART Log Started ===")
            continue

        match = pattern.match(line)
        if match:
            records.append({
                'local_ts': int(match.group(1)),
                'valid': match.group(2) == 'VALID',
                'device': match.group(3),
                'msg_id': int(match.group(4)),
                'data_type': match.group(5),
                'sensor': match.group(6),
                'value': float(match.group(7)),
                'unit': match.group(8),
                'esp_ts': int(match.group(9)),
                'checksum': match.group(10)
            })
        else:
            # Try to extract data from corrupted/concatenated lines
            # These lines have multiple messages merged together
            parts = line.split(',')
            local_ts = None
            if len(parts) >= 1:
                try:
                    local_ts = int(parts[0])
                except ValueError:
                    continue

            # Find all embedded valid data patterns in the corrupted line
            embedded_matches = embedded_pattern.findall(line)
            if embedded_matches:
                for ematch in embedded_matches:
                    try:
                        records.append({
                            'local_ts': local_ts,
                            'valid': False,  # Mark as recovered from invalid line
                            'device': ematch[0],
                            'msg_id': int(ematch[1]),
                            'data_type': ematch[2],
                            'sensor': ematch[3],
                            'value': float(ematch[4]),
                            'unit': ematch[5],
                            'esp_ts': int(ematch[6]),
                            'checksum': ematch[7]
                        })
                    except (ValueError, IndexError):
                        continue
            elif local_ts is not None:
                # No embedded data found, record as invalid with no value
                records.append({
                    'local_ts': local_ts,
                    'valid': False,
                    'device': 'thrust_test',
                    'msg_id': None,
                    'data_type': 'DATA',
                    'sensor': 'THST',
                    'value': None,
                    'unit': 'N',
                    'esp_ts': None,
                    'checksum': None
                })

    df = pd.DataFrame(records)

    if not df.empty:
        # Convert timestamps to relative time in seconds
        df['time_sec'] = (df['local_ts'] - df['local_ts'].min()) / 1000.0

    return df


def calculate_statistics(df: pd.DataFrame, valid_only: bool = False, use_absolute: bool = False) -> dict:
    """Calculate statistics for thrust data.

    Note: valid_only=False by default to include recovered data from corrupted UART lines.
    The 'valid' flag indicates transmission integrity, not data quality.
    """
    if df.empty:
        return {}

    data = df[df['valid']] if valid_only else df
    values = data['value'].dropna()

    # Apply absolute value transformation if enabled
    if use_absolute:
        values = values.abs()

    if values.empty:
        return {}

    # Calculate sample rate from timestamps
    if len(data) > 1:
        time_diffs = data['local_ts'].diff().dropna()
        avg_interval_ms = time_diffs.mean()
        sample_rate = 1000 / avg_interval_ms if avg_interval_ms > 0 else 0
    else:
        sample_rate = 0

    std_val = values.std()
    return {
        'count': len(values),
        'valid_count': len(df[df['valid']]),
        'invalid_count': len(df[~df['valid']]),
        'min': values.min(),
        'max': values.max(),
        'mean': values.mean(),
        'std': std_val if pd.notna(std_val) else 0.0,
        'median': values.median(),
        'range': values.max() - values.min(),
        'sample_rate_hz': sample_rate,
        'duration_sec': (df['local_ts'].max() - df['local_ts'].min()) / 1000.0
    }


# =============================================================================
# Plotly Figure Creation
# =============================================================================

def create_thrust_figure(df: pd.DataFrame, show_invalid: bool = False, use_absolute: bool = False) -> go.Figure:
    """Create interactive thrust vs time plot."""

    fig = go.Figure()

    if df.empty:
        fig.add_annotation(
            text="No data loaded. Upload a file or use --file argument.",
            xref="paper", yref="paper",
            x=0.5, y=0.5, showarrow=False,
            font=dict(size=16, color="#888")
        )
        return fig

    # Apply absolute value transformation if enabled
    plot_df = df.copy()
    if use_absolute:
        plot_df['value'] = plot_df['value'].abs()

    # Get all data with values (includes recovered data from corrupted lines)
    all_data_with_values = plot_df[plot_df['value'].notna()].sort_values('time_sec')
    invalid_data = plot_df[~plot_df['valid']].sort_values('time_sec')

    # Main thrust trace (ALL data with values - valid + recovered)
    # Use Scattergl (WebGL) for large datasets for better performance
    scatter_type = go.Scattergl if len(all_data_with_values) > 1000 else go.Scatter
    plot_mode = 'lines' if len(all_data_with_values) > 5000 else 'lines+markers'

    fig.add_trace(
        scatter_type(
            x=all_data_with_values['time_sec'],
            y=all_data_with_values['value'],
            mode=plot_mode,
            name='Thrust',
            line=dict(color='#00d4aa', width=1.5),
            marker=dict(size=4, color='#00d4aa'),
            hovertemplate=(
                '<b>Time:</b> %{x:.3f}s<br>'
                '<b>Thrust:</b> %{y:.3f} N<br>'
                '<b>Msg ID:</b> %{customdata[0]}<br>'
                '<b>ESP Time:</b> %{customdata[1]}ms'
                '<extra></extra>'
            ),
            customdata=all_data_with_values[['msg_id', 'esp_ts']].values
        )
    )

    # Highlight invalid/recovered data points (if enabled)
    if show_invalid and not invalid_data.empty:
        invalid_with_values = invalid_data[invalid_data['value'].notna()].sort_values('time_sec')
        if not invalid_with_values.empty:
            fig.add_trace(
                scatter_type(
                    x=invalid_with_values['time_sec'],
                    y=invalid_with_values['value'],
                    mode='markers',
                    name='Recovered (was invalid)',
                    marker=dict(size=8, color='#ff4757', symbol='x'),
                    hovertemplate=(
                        '<b>Time:</b> %{x:.3f}s<br>'
                        '<b>Thrust:</b> %{y:.3f} N<br>'
                        '<b>Status:</b> Recovered from corrupted line'
                        '<extra></extra>'
                    )
                )
            )

    # Statistics lines
    if not all_data_with_values.empty:
        mean_val = all_data_with_values['value'].mean()
        std_val = all_data_with_values['value'].std()

        fig.add_hline(
            y=mean_val, line_dash="dash", line_color="#ffa502",
            annotation_text=f"Mean: {mean_val:.3f} N"
        )
        # Only add std deviation lines if std is valid (not NaN, requires >1 sample)
        if pd.notna(std_val) and std_val > 0:
            fig.add_hline(
                y=mean_val + std_val, line_dash="dot", line_color="#ffa502",
                opacity=0.5
            )
            fig.add_hline(
                y=mean_val - std_val, line_dash="dot", line_color="#ffa502",
                opacity=0.5
            )

    # Layout
    fig.update_layout(
        title='Thrust vs Time',
        template='plotly_dark',
        paper_bgcolor='#1a1a2e',
        plot_bgcolor='#16213e',
        font=dict(family='monospace', color='#eee'),
        legend=dict(
            orientation='h',
            yanchor='bottom',
            y=1.02,
            xanchor='right',
            x=1
        ),
        margin=dict(l=60, r=40, t=60, b=50),
        hovermode='x unified',
        xaxis=dict(
            title='Time (seconds)',
            gridcolor='#2a2a4a',
            rangeslider=dict(visible=False),
            type='linear'
        ),
        yaxis=dict(
            title='Thrust (N)',
            gridcolor='#2a2a4a'
        ),
        dragmode='zoom'
    )

    return fig


# =============================================================================
# UI Components
# =============================================================================

def create_stats_panel(stats: dict) -> list:
    """Create statistics display panel."""
    if not stats:
        return [html.P('No data loaded', style={'color': '#666'})]

    stat_items = [
        ('Total Samples', f"{stats.get('count', 0):,}"),
        ('Valid', f"{stats.get('valid_count', 0):,}"),
        ('Invalid', f"{stats.get('invalid_count', 0):,}"),
        ('', ''),  # Spacer
        ('Min Thrust', f"{stats.get('min', 0):.3f} N"),
        ('Max Thrust', f"{stats.get('max', 0):.3f} N"),
        ('Range', f"{stats.get('range', 0):.3f} N"),
        ('', ''),  # Spacer
        ('Mean', f"{stats.get('mean', 0):.3f} N"),
        ('Median', f"{stats.get('median', 0):.3f} N"),
        ('Std Dev', f"{stats.get('std', 0):.3f} N"),
        ('', ''),  # Spacer
        ('Sample Rate', f"{stats.get('sample_rate_hz', 0):.1f} Hz"),
        ('Duration', f"{stats.get('duration_sec', 0):.2f} s"),
    ]

    elements = []
    for label, value in stat_items:
        if not label:
            elements.append(html.Hr(style={'borderColor': '#333', 'margin': '10px 0'}))
        else:
            elements.append(
                html.Div([
                    html.Span(label + ':', style={'color': '#888'}),
                    html.Span(value, style={'color': '#fff', 'float': 'right', 'fontWeight': 'bold'})
                ], style={'marginBottom': '8px', 'fontSize': '13px'})
            )

    return elements


def create_data_table(df: pd.DataFrame) -> dash_table.DataTable:
    """Create data table component with virtualization for large datasets."""
    if df.empty:
        return html.P('No data loaded', style={'color': '#666'})

    # Sort by time and select columns for display
    display_df = df.sort_values('time_sec')[['time_sec', 'valid', 'msg_id', 'value', 'esp_ts']].copy()
    display_df.columns = ['Time (s)', 'Valid', 'Msg ID', 'Thrust (N)', 'ESP Time (ms)']

    # Round for display
    display_df['Time (s)'] = display_df['Time (s)'].round(3)
    display_df['Thrust (N)'] = display_df['Thrust (N)'].round(3)

    return dash_table.DataTable(
        data=display_df.to_dict('records'),
        columns=[{'name': col, 'id': col} for col in display_df.columns],
        # Virtualization for large datasets
        virtualization=True,
        fixed_rows={'headers': True},
        style_table={
            'overflowX': 'auto',
            'height': 'calc(100vh - 250px)',
            'minHeight': '400px'
        },
        style_header={
            'backgroundColor': '#16213e',
            'color': '#00d4aa',
            'fontWeight': 'bold',
            'border': '1px solid #333',
            'position': 'sticky',
            'top': 0
        },
        style_cell={
            'backgroundColor': '#1a1a2e',
            'color': '#ddd',
            'border': '1px solid #333',
            'textAlign': 'center',
            'padding': '10px',
            'minWidth': '100px',
            'maxWidth': '180px'
        },
        style_data_conditional=[
            {
                'if': {'filter_query': '{Valid} eq false'},
                'backgroundColor': '#3d1f1f',
                'color': '#ff4757'
            }
        ],
        filter_action='native',
        sort_action='native',
        sort_mode='multi',
        page_action='none'  # Disable pagination, use virtualization instead
    )


# =============================================================================
# Dash Application
# =============================================================================

def create_app(initial_file: str = None) -> Dash:
    """Create and configure the Dash application."""

    app = Dash(
        __name__,
        title='Thrust Test Visualizer',
        update_title='Loading...'
    )

    # Load initial data if file provided
    initial_df = pd.DataFrame()
    initial_stats = {}
    initial_file_info = ''
    if initial_file:
        try:
            with open(initial_file, 'r') as f:
                content = f.read()
            initial_df = parse_thrust_data(content)
            initial_stats = calculate_statistics(initial_df)
            initial_file_info = f'Loaded: {Path(initial_file).name} ({len(initial_df)} records)'
            print(f"Parsed {len(initial_df)} records from {initial_file}")
        except Exception as e:
            print(f"Error loading file: {e}")
            initial_file_info = f'Error: {str(e)}'

    # App Layout
    app.layout = html.Div([
        # Header
        html.Div([
            html.H1('Thrust Test Data Visualizer',
                   style={'margin': '0', 'color': '#00d4aa'}),
            html.P('Interactive analysis of ESP32/Teensy thrust measurements',
                  style={'margin': '5px 0 0 0', 'color': '#888'})
        ], style={'padding': '20px', 'backgroundColor': '#0f0f23', 'borderBottom': '2px solid #00d4aa'}),

        # Main container
        html.Div([
            # Left panel - Controls & Stats
            html.Div([
                # File Upload (collapsible)
                html.Details([
                    html.Summary('Data Source', style={
                        'color': '#00d4aa',
                        'cursor': 'pointer',
                        'fontWeight': 'bold',
                        'fontSize': '16px',
                        'marginBottom': '10px'
                    }),
                    dcc.Upload(
                        id='upload-data',
                        children=html.Div([
                            'Drag & Drop or ',
                            html.A('Select File', style={'color': '#00d4aa', 'cursor': 'pointer'})
                        ]),
                        style={
                            'width': '100%',
                            'height': '60px',
                            'lineHeight': '60px',
                            'borderWidth': '2px',
                            'borderStyle': 'dashed',
                            'borderRadius': '10px',
                            'borderColor': '#444',
                            'textAlign': 'center',
                            'backgroundColor': '#1a1a2e',
                            'color': '#888'
                        },
                        multiple=False
                    ),
                    html.Div(id='file-info', children=initial_file_info, style={'marginTop': '10px', 'color': '#00d4aa' if initial_file_info else '#888', 'fontSize': '12px'})
                ], open=not bool(initial_file_info), style={'marginBottom': '20px'}),

                # Filters
                html.Div([
                    html.H3('Display Options', style={'color': '#00d4aa'}),
                    dcc.Checklist(
                        id='show-invalid',
                        options=[{'label': ' Show Invalid Data Points', 'value': 'show'}],
                        value=[],
                        style={'color': '#ddd', 'marginBottom': '10px'}
                    ),
                    dcc.Checklist(
                        id='use-absolute',
                        options=[{'label': ' Use Absolute Values (for compression load cells)', 'value': 'abs'}],
                        value=['abs'],  # Default ON since load cell reads negative for thrust
                        style={'color': '#ddd'}
                    )
                ], style={'marginBottom': '20px'}),

                # Statistics Panel
                html.Div([
                    html.H3('Statistics', style={'color': '#00d4aa'}),
                    html.Div(id='stats-panel', children=create_stats_panel(initial_stats))
                ], style={'marginBottom': '20px'}),

                # Export Options
                html.Div([
                    html.H3('Export', style={'color': '#00d4aa'}),
                    html.Button('Download CSV', id='btn-csv', n_clicks=0,
                               style={'marginRight': '10px', 'padding': '10px 20px',
                                     'backgroundColor': '#00d4aa', 'border': 'none',
                                     'borderRadius': '5px', 'cursor': 'pointer',
                                     'color': '#1a1a2e', 'fontWeight': 'bold'}),
                    html.Button('Download HTML', id='btn-html', n_clicks=0,
                               style={'padding': '10px 20px',
                                     'backgroundColor': '#ffa502', 'border': 'none',
                                     'borderRadius': '5px', 'cursor': 'pointer',
                                     'color': '#1a1a2e', 'fontWeight': 'bold'}),
                    dcc.Download(id='download-csv'),
                    dcc.Download(id='download-html')
                ])

            ], style={
                'width': '280px',
                'padding': '20px',
                'backgroundColor': '#0f0f23',
                'borderRight': '1px solid #333',
                'height': 'calc(100vh - 100px)',
                'overflowY': 'auto'
            }),

            # Right panel - Graph & Table
            html.Div([
                # Tabs for Graph and Table
                dcc.Tabs([
                    dcc.Tab(label='Graph', children=[
                        dcc.Graph(
                            id='thrust-graph',
                            figure=create_thrust_figure(initial_df),
                            style={'height': 'calc(100vh - 200px)'},
                            config={
                                'displayModeBar': True,
                                'displaylogo': False,
                                'modeBarButtonsToAdd': ['drawline', 'eraseshape'],
                                'toImageButtonOptions': {
                                    'format': 'png',
                                    'filename': 'thrust_data',
                                    'height': 800,
                                    'width': 1200,
                                    'scale': 2
                                }
                            }
                        )
                    ], style={'backgroundColor': '#1a1a2e', 'color': '#ddd'},
                       selected_style={'backgroundColor': '#16213e', 'color': '#00d4aa', 'borderTop': '2px solid #00d4aa'}),

                    dcc.Tab(label='Data Table', children=[
                        html.Div(id='data-table-container', children=create_data_table(initial_df), style={'padding': '20px'})
                    ], style={'backgroundColor': '#1a1a2e', 'color': '#ddd'},
                       selected_style={'backgroundColor': '#16213e', 'color': '#00d4aa', 'borderTop': '2px solid #00d4aa'})
                ], style={'backgroundColor': '#0f0f23'})
            ], style={
                'flex': '1',
                'backgroundColor': '#1a1a2e',
                'overflow': 'hidden'
            })

        ], style={
            'display': 'flex',
            'height': 'calc(100vh - 100px)'
        }),

        # Hidden store for data
        dcc.Store(id='stored-data', data=initial_df.to_json(orient='split') if not initial_df.empty else None)

    ], style={
        'fontFamily': 'monospace',
        'backgroundColor': '#0f0f23',
        'minHeight': '100vh',
        'margin': '0'
    })

    return app


# =============================================================================
# Callbacks
# =============================================================================

def register_callbacks(app: Dash):
    """Register all Dash callbacks."""

    @app.callback(
        [Output('stored-data', 'data'),
         Output('file-info', 'children')],
        Input('upload-data', 'contents'),
        State('upload-data', 'filename')
    )
    def upload_file(contents, filename):
        if contents is None:
            # Don't update if no file uploaded - preserves initial data
            return no_update, no_update

        try:
            # Parse base64 content (format: "data:type;base64,<data>")
            if ',' not in contents:
                return no_update, f'Error: Invalid file format'

            content_type, content_string = contents.split(',', 1)
            decoded = base64.b64decode(content_string)
            content = decoded.decode('utf-8')
            df = parse_thrust_data(content)

            if df.empty:
                return no_update, f'No valid data found in {filename}'

            return df.to_json(orient='split'), f'Loaded: {filename} ({len(df)} records)'
        except (ValueError, UnicodeDecodeError) as e:
            return no_update, f'Error parsing file: {str(e)}'
        except Exception as e:
            return no_update, f'Error: {str(e)}'

    @app.callback(
        Output('thrust-graph', 'figure'),
        [Input('stored-data', 'data'),
         Input('show-invalid', 'value'),
         Input('use-absolute', 'value')]
    )
    def update_graph(json_data, show_invalid, use_absolute):
        if json_data is None:
            return create_thrust_figure(pd.DataFrame())

        df = pd.read_json(io.StringIO(json_data), orient='split')
        show = 'show' in show_invalid if show_invalid else False
        use_abs = 'abs' in use_absolute if use_absolute else False
        return create_thrust_figure(df, show_invalid=show, use_absolute=use_abs)

    @app.callback(
        Output('stats-panel', 'children'),
        [Input('stored-data', 'data'),
         Input('use-absolute', 'value')]
    )
    def update_stats(json_data, use_absolute):
        if json_data is None:
            return create_stats_panel({})

        df = pd.read_json(io.StringIO(json_data), orient='split')
        use_abs = 'abs' in use_absolute if use_absolute else False
        stats = calculate_statistics(df, use_absolute=use_abs)
        return create_stats_panel(stats)

    @app.callback(
        Output('data-table-container', 'children'),
        Input('stored-data', 'data')
    )
    def update_table(json_data):
        if json_data is None:
            return html.P('No data loaded', style={'color': '#666'})

        df = pd.read_json(io.StringIO(json_data), orient='split')
        return create_data_table(df)

    @app.callback(
        Output('download-csv', 'data'),
        Input('btn-csv', 'n_clicks'),
        State('stored-data', 'data'),
        prevent_initial_call=True
    )
    def download_csv(n_clicks, json_data):
        if json_data is None:
            return no_update

        df = pd.read_json(io.StringIO(json_data), orient='split')
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        return dcc.send_data_frame(df.to_csv, f'thrust_data_{timestamp}.csv', index=False)

    @app.callback(
        Output('download-html', 'data'),
        Input('btn-html', 'n_clicks'),
        State('stored-data', 'data'),
        State('show-invalid', 'value'),
        State('use-absolute', 'value'),
        prevent_initial_call=True
    )
    def download_html(n_clicks, json_data, show_invalid, use_absolute):
        if json_data is None:
            return no_update

        df = pd.read_json(io.StringIO(json_data), orient='split')
        show = 'show' in show_invalid if show_invalid else False
        use_abs = 'abs' in use_absolute if use_absolute else False
        fig = create_thrust_figure(df, show_invalid=show, use_absolute=use_abs)

        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        html_content = fig.to_html(include_plotlyjs='cdn', full_html=True)

        return dict(content=html_content, filename=f'thrust_graph_{timestamp}.html')


# =============================================================================
# Main Entry Point
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Thrust Test Data Visualizer - Interactive web dashboard for thrust measurements',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
    %(prog)s --file data.txt          Load specific file
    %(prog)s --file data.txt --port 8080  Custom port
    %(prog)s                          Start with file upload interface

Data Format:
    Expected format: <timestamp>,<VALID|INVALID>,$thrust_test,<id>,DATA,THST,<value>,N,<esp_ts>*<checksum>
        '''
    )

    parser.add_argument(
        '--file', '-f',
        type=str,
        help='Path to thrust data file (.txt)',
        metavar='FILE'
    )

    parser.add_argument(
        '--port', '-p',
        type=int,
        default=8050,
        help='Port to run the server on (default: 8050)'
    )

    parser.add_argument(
        '--host',
        type=str,
        default='127.0.0.1',
        help='Host to bind to (default: 127.0.0.1)'
    )

    parser.add_argument(
        '--debug',
        action='store_true',
        help='Enable debug mode'
    )

    args = parser.parse_args()

    # Validate file if provided
    if args.file:
        file_path = Path(args.file)
        if not file_path.exists():
            print(f"Error: File not found: {args.file}")
            return 1
        if not file_path.is_file():
            print(f"Error: Not a file: {args.file}")
            return 1

    # Create and run app
    app = create_app(initial_file=args.file)
    register_callbacks(app)

    print(f"\n{'='*60}")
    print("  Thrust Test Data Visualizer")
    print(f"{'='*60}")
    print(f"  Server running at: http://{args.host}:{args.port}")
    if args.file:
        print(f"  Loaded file: {args.file}")
    else:
        print("  Upload a file using the web interface")
    print(f"{'='*60}\n")

    app.run(host=args.host, port=args.port, debug=args.debug)
    return 0


if __name__ == '__main__':
    exit(main())
