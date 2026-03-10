# 📦 Sistema de Gerenciamento de Estoque (Simulador WMS)

Um sistema desenvolvido em C como projeto acadêmico para simular a operação de um armazém logístico (Warehouse Management System). O programa vai além do cadastro básico de produtos, implementando controle de fluxo de estoque e um sistema inteligente de roteamento para a coleta de itens.

## 🚀 Funcionalidades Principais

* **Gestão de Inventário (CRUD):** Cadastro, edição, remoção e listagem detalhada de produtos organizados em tabela.
* **Controle Operacional:** Bloqueio temporário de itens (com opção de reversão) e processamento de pedidos em lote.
* **Navegação Logística:** Assistente interativo que calcula a rota mais curta para o operador buscar um produto dentro do galpão.
* **Persistência de Dados:** Salvamento e carregamento automático do inventário em um arquivo de texto genérico (`estoque.txt`).

## 🧠 Estruturas de Dados e Algoritmos Aplicados

Este projeto é uma aplicação prática de fundamentos de Ciência da Computação:

* **Pilhas (Stacks):** Utilizadas para registrar operações de bloqueio de estoque, permitindo a reversão da última ação (LIFO - Last In, First Out).
* **Filas (Queues):** Gerenciam os pedidos dos clientes garantindo o processamento por ordem de chegada (FIFO - First In, First Out).
* **Grafos e Matrizes de Adjacência:** Mapeamento do layout do armazém, conectando áreas como WMS, Blocos e Área Externa.
* **Algoritmo de Dijkstra:** Motor de busca responsável por encontrar o caminho mais rápido e curto entre a posição do usuário e o local de armazenamento do produto.
* **Alocação Dinâmica de Memória:** Uso contínuo de ponteiros, `malloc` e `realloc` para otimizar o consumo de memória conforme o estoque varia.
