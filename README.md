# Marco-3
# Projeto Coprocessador - Sistemas Digitais (PBL)
Aplicação final do sistema de classificação de dígitos com interface VGA,
integrando o co-processador ELM desenvolvido nos marcos anteriores.

---

## Sumário
- [1. Introdução](#1-introdução)
- [2. Sobre o Projeto](#2-sobre-o-projeto)
- [3. Declaração do Problema](#3-declaração-do-problema)
  - [3.1 Contexto](#31-contexto)
  - [3.2 Requisitos Principais](#32-requisitos-principais)
- [4. Arquitetura da Solução](#4-arquitetura-da-solução)
  - [4.1 Visão Geral](#41-visão-geral)
  - [4.2 Fluxo de Execução](#42-fluxo-de-execução)
- [5. Fundamentação Teórica](#5-fundamentação-teórica)
  - [5.1 IP-Core VGA](#51-ip-core-vga)
  - [5.2 Modos de Operação](#52-modos-de-operação)
  - [5.3 Métricas de Benchmark](#53-métricas-de-benchmark)
- [6. Materiais e Métodos](#6-materiais-e-métodos)
  - [6.1 DE1-SoC](#61-de1-soc)
  - [6.2 IP-Core VGA](#62-ip-core-vga)
  - [6.3 Driver Assembly — Marco 2](#63-driver-assembly--marco-2)
  - [6.4 Metodologia](#64-metodologia)
- [7. Descrição da Solução](#7-descrição-da-solução)
  - [7.1 Modo Arquivo](#71-modo-arquivo)
  - [7.2 Modo Desenho](#72-modo-desenho)
  - [7.3 Modo Benchmark](#73-modo-benchmark)
  - [7.4 Integração com o Driver do Marco 2](#74-integração-com-o-driver-do-marco-2)
- [8. Modo de Uso](#8-modo-de-uso)
- [9. Testes e Resultados](#9-testes-e-resultados)
- [10. Referências](#10-referências)

---

## 1. Introdução

Esse repositório é referente ao Marco 3 da disciplina de SD (Sistemas 
Digitais) - TEC499. Depois de dois marcos focados no hardware e na 
comunicação com a FPGA, chegamos na etapa final: desenvolver a aplicação 
em C que o usuário vai usar de verdade para classificar dígitos.

No Marco 1 o co-processador ELM foi implementado na FPGA da DE1-SoC, e 
no Marco 2 nosso grupo desenvolveu o driver em Assembly ARM que faz a 
comunicação entre o HPS e esse hardware via MMIO. Com tudo isso funcionando 
e validado, o Marco 3 fecha o ciclo com uma aplicação completa, que inclui 
interface visual pelo VGA e três formas diferentes de uso.

---

## 2. Sobre o Projeto

O sistema classifica dígitos manuscritos de 0 a 9 usando o modelo ELM 
rodando diretamente no hardware da FPGA. A aplicação desse marco é a parte 
que o usuário enxerga: ela recebe as entradas, prepara os dados e mostra 
o resultado. Por baixo dos panos, quem faz o trabalho pesado ainda é o 
driver do Marco 2, que continua sendo usado do jeito que foi entregue.

Nesse marco aqui é a integração com o IP-Core VGA, disponibilizado para a 
disciplina, que permite tanto mostrar imagens no monitor quanto capturar 
o que o usuário desenha com o mouse para depois mandar ao co-processador.

---

## 3. Declaração do Problema

### 3.1 Contexto

No Marco 2 o driver ficou funcional, com acurácia acima de 80% nos testes, 
mas a inferência só rodava através de um programa de testes simples em C, 
sem nenhuma interface visual e sem flexibilidade para o usuário escolher 
a imagem. Para concluir o projeto de forma completa, era necessário ir além 
disso.

O Marco 3 entra justamente nessa lacuna. O desafio principal foi integrar 
o IP-Core VGA ao projeto já existente sem quebrar o que estava funcionando, 
além de garantir que a imagem exibida no monitor fosse exatamente a mesma 
enviada ao co-processador. O modo de desenho via mouse também trouxe um 
trabalho extra, já que o que o usuário desenha na tela precisa ser 
convertido para o formato que o hardware espera. Por fim, o modo benchmark 
exigiu atenção na medição de tempo para que as métricas calculadas 
refletissem o comportamento real do sistema.

### 3.2 Requisitos Principais

**Integração do IP-Core VGA ao projeto existente**

**Modo de inferência a partir de imagem lida de arquivo**

**Modo de inferência a partir de desenho capturado via mouse e VGA**

**Modo de validação/benchmark com N imagens**

**Interface via linha de comando** (caminho da imagem e parâmetros)

**Exibição da imagem a ser inferida via VGA**

**Comunicação com o co-processador via driver do Marco 2**

**Cálculo de acurácia (%), latência média, desvio padrão e throughput (imagens/s)**

**Log de resultados salvo em CSV**