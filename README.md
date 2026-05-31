# VCellsWar — Multiplayer RTS on Unreal Engine 5.7 (C++)

A networked Real-Time Strategy game built from scratch using the UE5 C++ API and Steam Online Subsystem.

## 🚀 Current Features (Implemented)
* **Steam Integration:** Full lobby system featuring matchmaking (Host/Join match via Steam overlay).
* **Networked Map Generation:** Deterministic runtime map generation synced across all clients via a replicated seed.
* **Graph & Region Generation (C++):** Map partitioning and tactical region segmentation computed natively in C++ using a custom implementation of the **Bowyer–Watson algorithm** for Delaunay Triangulation, subsequently dualized into a Voronoi Diagram. Features an interactive, low-latency map preview inside the multiplayer lobby.
* **Architecture:** Clean separation of architectural gameplay logic in C++ and visual/audio representations in Blueprints.

## 🛠️ Tech Stack & Concepts Used
* Unreal Engine 5 (C++)
* Steam Online Subsystem (OSS)
* Component-Based Architecture
* Network Replication & RPCs (Client/Server Authoritative model)

## 📐 Detailed Technical Architecture of Map Generation
1. **Lobby & Seed Sync:** The map seed is selected in the lobby and replicated to all clients immediately upon match start.
2. **Voronoi Graph Generation (C++):** Region segmentation is calculated on C++ using the Bowyer-Watson algorithm. It handles the spatial layout and generates the graph data used for the interactive lobby preview.
3. **Terrain & Displacement (Blueprints):** Heightmap and terrain relief are generated deterministically on both server and clients using Perlin Noise via the UE5 Dynamic Mesh Component.
4. **Actor Placement (WIP):** Server-side authoritative spawning of strategic nodes and resources based on the pre-calculated C++ Voronoi graph coordinates to prevent desynchronization.

## 📅 Roadmap (In Progress)
- [ ] Implement networked unit selection (Marquee/Box selection).
- [ ] Add basic AI for neutral/enemy cells.
- [ ] Integrate Gameplay Ability System (GAS) for cell mutations and skills.

## 📄 License
This project is licensed under the MIT License - see the LICENSE file for details.
