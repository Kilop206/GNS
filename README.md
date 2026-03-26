# KNS — Kilop's Network Simulator

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

- Deterministic execution[
- Results reprodutction
- Modular extensibility
- Solid base for future network abstractions

---

## Current Architecture
	KNS/
	|-- core/ # Main components for the simulation motor
	|-- app/ # Main executable
	|-- tests / # Unitaty tests
	|-- topologies / # JSON files of topologies models
	|-- CMakeLists.txt
	
---

## Used Technologies

- C++20
- CMake
- Catch2 (unitary tests)
- Ctest

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

---
---

# KNS — Kilop's Network Simulator

## Overview

KNS é um simulador de redes orientado a eventos, com foco em determinismo, modularidade e base científica sólida.

O projeto está sendo desenvolvido incrementalmente, priorizando:

- Arquitetura limpa
- Separação clara de responsabilidades
- Testes automatizados
- Determinismo forte na simulação

---

## Objetivo

Construir um motor de simulação de redes baseado em eventos discretos que permita:

- Execução determinística
- Reprodutibilidade de resultados
- Extensibilidade modular
- Base sólida para futuras abstrações de rede

---

## Arquitetura Atual
	KNS/
	|-- core/ # Componentes principais do motor de simulação
	|-- app/ # Executável principal
	|-- tests/ # Testes unitários
	|-- topologies/ # Arquivos JSON de modelos de topologias
	|-- CMakeLists.txt

---

## Tecnologias Utilizadas

- C++20
- CMake
- Catch2 (testes unitários)
- CTest

---

## Como Compilar

### 1️º Gerar build

```bash
cmake -S . -B build
```

### 2️º Compilar
```bash
cmake --build build
```

Como o projeto usa gerador multi-config (Visual Studio):
### 3️º Executar testes
```bash
ctest -C Debug --output-on-failure
