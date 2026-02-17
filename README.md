# graphcrawler

**Course Assignment – Parallel Programming**  
**Author:** Michael Cashion

## Overview
This project implements a **breadth-first search (BFS)** traversal over a graph exposed through a **web API**.  
The graph represents actors and movies, where edges are returned dynamically by querying a remote server.

Two implementations are provided:
- **Sequential level-by-level BFS**
- **Parallel level-by-level BFS** using multithreading

The parallel version expands all nodes at the same BFS depth concurrently while ensuring correctness using mutexes.

---

## Graph API
The program queries neighbors using the endpoint:

