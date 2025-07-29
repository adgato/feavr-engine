#!/usr/bin/env python3
"""
Find redundant edges in a directed graph - edges where alternative paths exist.
Usage: python redundant_edges.py < input.dot > output.dot
"""

import sys
import networkx as nx
from networkx.drawing.nx_agraph import read_dot, write_dot

def find_redundant_edges(G):
    """
    Find all edges A->C where removing the edge still leaves a path from A to C.
    Returns a set of redundant edges.
    """
    redundant_edges = set()
    
    # Check each edge
    for source, target in G.edges():
        # Create a copy of the graph without this edge
        G_temp = G.copy()
        G_temp.remove_edge(source, target)
        
        # Check if there's still a path from source to target
        try:
            if nx.has_path(G_temp, source, target):
                redundant_edges.add((source, target))
                print(f"Redundant edge found: {source} -> {target}", file=sys.stderr)
        except nx.NetworkXNoPath:
            pass
        except nx.exception.NodeNotFound:
            pass
    
    return redundant_edges

def find_cycles(G):
    """Find all edges that are part of cycles."""
    cycle_edges = set()
    
    try:
        # Find all strongly connected components with more than one node
        sccs = [scc for scc in nx.strongly_connected_components(G) if len(scc) > 1]
        
        for scc in sccs:
            # Get the subgraph for this SCC
            subgraph = G.subgraph(scc)
            # All edges in this subgraph are part of cycles
            cycle_edges.update(subgraph.edges())
            
        # Also find self-loops
        cycle_edges.update(nx.selfloop_edges(G))
        
    except Exception as e:
        print(f"Error finding cycles: {e}", file=sys.stderr)
    
    return cycle_edges

def apply_colors(G, redundant_edges, cycle_edges):
    """Apply color attributes to edges."""
    
    # Set default styling for all nodes (dark mode)
    for node in G.nodes():
        G.nodes[node].update({
            'shape': 'box',
            'style': 'filled',
            'fillcolor': '#2d3748',
            'fontcolor': '#e2e8f0',
            'color': '#4a5568',
            'fontsize': '10'
        })
    
    # Set default styling for all edges (dark mode) - use edge data access
    print(f"Processing edges for styling...", file=sys.stderr)
    
    for source, target, data in G.edges(data=True):
        data.update({
            'fontsize': '8',
            'fontcolor': '#a0aec0',
            'color': '#718096'
        })
    
    # Color cycle edges red (higher priority)
    for source, target in cycle_edges:
        if G.has_edge(source, target):
            edge_data = G[source][target]
            if isinstance(edge_data, dict):
                edge_data.update({
                    'color': '#f56565',
                    'penwidth': '2'
                })
            else:
                # Handle multigraph case
                for key in edge_data:
                    edge_data[key].update({
                        'color': '#f56565',
                        'penwidth': '2'
                    })
            print(f"Cycle edge: {source} -> {target}", file=sys.stderr)
    
    # Color redundant edges yellow (but don't override cycle edges)
    for source, target in redundant_edges:
        if G.has_edge(source, target) and (source, target) not in cycle_edges:
            edge_data = G[source][target]
            if isinstance(edge_data, dict):
                edge_data.update({
                    'color': '#ffd700',
                    'penwidth': '2'
                })
            else:
                # Handle multigraph case
                for key in edge_data:
                    edge_data[key].update({
                        'color': '#ffd700',
                        'penwidth': '2'
                    })
    
    # Set graph attributes for dark mode
    G.graph.update({
        'rankdir': 'TB',
        'overlap': 'false',
        'splines': 'true',
        'bgcolor': '#1e1e1e'
    })

def main():
    try:
        # Read the graph from stdin
        print("Reading graph from stdin...", file=sys.stderr)
        G = read_dot(sys.stdin)
        
        print(f"Graph has {G.number_of_nodes()} nodes and {G.number_of_edges()} edges", file=sys.stderr)
        
        # Find cycle edges first
        print("Finding cycle edges...", file=sys.stderr)
        cycle_edges = find_cycles(G)
        print(f"Found {len(cycle_edges)} cycle edges", file=sys.stderr)
        
        # Find redundant edges
        print("Finding redundant edges...", file=sys.stderr)
        redundant_edges = find_redundant_edges(G)
        print(f"Found {len(redundant_edges)} redundant edges", file=sys.stderr)
        
        # Apply colors
        print("Applying colors...", file=sys.stderr)
        apply_colors(G, redundant_edges, cycle_edges)
        
        # Write the result to stdout
        print("Writing result to stdout...", file=sys.stderr)
        write_dot(G, sys.stdout)
        
        print("Done!", file=sys.stderr)
        print(f"Red edges: {len(cycle_edges)} (cycles)", file=sys.stderr)
        print(f"Yellow edges: {len(redundant_edges - cycle_edges)} (redundant)", file=sys.stderr)
        print(f"Gray edges: {G.number_of_edges() - len(cycle_edges) - len(redundant_edges - cycle_edges)} (essential)", file=sys.stderr)
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()