# KNS — Kilop's Network Simulator

## Quick Start
```bash
.\scripts\demo.ps1
```

Prerequisits:
 - CMake
 - Visual Studio
 - Python 3

## Overview

KNS is a network simulator oriented to events, focusing on determinism, modularity and solid scientific base.

The project is being developed incrementaly, prioritizing:
- Clean Architecture
- Clear separation of responsabilities
- Automatizated tests
- Strong determinism on simulation

---

## Goal

Build the motor of a network simulator based on discret events that lets:

- Deterministic execution
- Results reprodutction
- Modular extensibility
- Solid base for future network abstractions

---

## Current Architecture
	KNS/
	|-- app/ # Main executable
	|-- core/ # Main components for the simulation motor
	|-- docs/ # Documents explaining the project
	|-- results/ # Contains the results of previous experiments
	|-- scripts/ # Scripts that automatically run and create graphs
	|-- tests / # Unitaty tests
	|-- topologies / # JSON files of topologies models
	|-- CMakeLists.txt
	
---

## Experiments

On this section we will describe how you should run the scripts in order to have automated tests for your topology

### 1st Enter scripts directory
```bash
cd scripts
```

### 2nd Run run_all.ps1
```bash
.\run_all.ps1
```

### 3rd Run plot_results.py
```bash
py plot_results.py
```

## Results experiments

![Loss Rate vs Loss Probability](results/loss_rate_vs_loss_prob.png)
![Latency vs Loss Probability](results/latency_vs_loss_prob.png)

[Read the full analysis](docs/experiments.md)

---

## Used Technologies

- C++20
- CMake
- Catch2 (unit tests)
- Ctest
- ImGui

---

## How to Compile

### 1st Generate build
```bash
cmake -S . -B build
```

### 2nd Compile
```bash
cmake --build build
```

Since the project uses multi-config generator (Visual Studio):
### 3️rd Execute Tests
```bash
ctest -C Debug --output-on-failure
```