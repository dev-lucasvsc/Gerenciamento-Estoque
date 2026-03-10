#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARQUIVO_ESTOQUE "estoque.txt"
#define MAX_PILHA 100
#define MAX_FILA 50
#define NUM_LOCAIS 7
#define MAX_RUAS 13
#define INFINITO 99999

#define MESA_OPERACOES 0
#define WMS 1
#define AREA_EXTERNA 2
#define BLOCO_D 3
#define BLOCO_F 4
#define MOSTRUARIO 5
#define BLOCO_ACO 6

// Enum para o Menu Principal
enum Menu { 
    CADASTRAR = 1, EDITAR, REMOVER, RELATORIO, BLOQUEAR, 
    REVERTER, ADD_PEDIDO, PROC_FILA, NAVEGAR, LIMPAR, SAIR 
};

typedef struct Sestoque {
    char nomeProduto[30], marcaProd[15];
    int adm, quantidade;
    long codBarras, codOriginal;
    char localArmazenamento[20];
    int ruaArmazenamento;
} estoque;

typedef struct sBlock { int adm; int quantidadeBloqueada; } Bloqueio;
typedef struct sPilhaBlock { Bloqueio operacoes[MAX_PILHA]; int topo; } PilhaBloqueio;
typedef struct sPedido { int adm; int quantidadeSolicitada; } Pedido;
typedef struct sFilaPedidos { Pedido itens[MAX_FILA]; int inicio, fim, contador; } FilaPedidos;
typedef struct { const char* nome; int numRuas; int matrizAdj[MAX_RUAS][MAX_RUAS]; } LayoutLocal;

LayoutLocal layouts[NUM_LOCAIS];
const char* nomesLocais[NUM_LOCAIS] = {
    "Mesa de Operacoes", "WMS", "Area Externa", "Bloco D",
    "Bloco F", "Mostruario", "Bloco de Aco"
};

// ================= UTILITÁRIOS E LEITURA =================

void limparBufferEntrada() { int c; while ((c = getchar()) != '\n' && c != EOF); }
void pressioneEnterParaContinuar() { printf("\nPressione Enter para continuar..."); limparBufferEntrada(); }

void lerString(const char* prompt, char* destino, int tamanho) { 
    printf("%s", prompt); 
    if (fgets(destino, tamanho, stdin) != NULL) { 
        destino[strcspn(destino, "\n")] = 0; 
    } 
}

int lerInteiro(const char* prompt) { 
    char buffer[100]; int valor; 
    while (1) { 
        printf("%s", prompt); 
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) { 
            if (sscanf(buffer, "%d", &valor) == 1) return valor; 
        } 
        printf("Entrada invalida! Por favor, digite um numero inteiro.\n"); 
    } 
}

long lerLong(const char* prompt) { 
    char buffer[100]; long valor; 
    while (1) { 
        printf("%s", prompt); 
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) { 
            if (sscanf(buffer, "%ld", &valor) == 1) return valor; 
        } 
        printf("Entrada invalida! Por favor, digite um numero longo.\n"); 
    } 
}

// ================= ESTRUTURAS DE DADOS =================

void inicializarPilha(PilhaBloqueio *pilha) { pilha->topo = -1; }
int pop(PilhaBloqueio *pilha, Bloqueio *op) { if (pilha->topo == -1) return 0; *op = pilha->operacoes[(pilha->topo)--]; return 1; }
int push(PilhaBloqueio *pilha, Bloqueio op) { if (pilha->topo == MAX_PILHA - 1) return 0; pilha->operacoes[++(pilha->topo)] = op; return 1; }

void inicializarFila(FilaPedidos *fila) { fila->inicio = 0; fila->fim = -1; fila->contador = 0; }
int enfileirar(FilaPedidos *fila, Pedido n) { if (fila->contador == MAX_FILA) return 0; fila->fim=(fila->fim+1)%MAX_FILA; fila->itens[fila->fim]=n; fila->contador++; return 1; }
int desenfileirar(FilaPedidos *fila, Pedido *p) { if (fila->contador == 0) return 0; *p=fila->itens[fila->inicio]; fila->inicio=(fila->inicio+1)%MAX_FILA; fila->contador--; return 1; }

// ================= MAPAS E ROTAS (GRAFOS) =================

void inicializarMapasLocais() {
    for (int i = 0; i < NUM_LOCAIS; i++) {
        layouts[i].nome = nomesLocais[i]; layouts[i].numRuas = 0;
        for (int j = 0; j < MAX_RUAS; j++) for (int k = 0; k < MAX_RUAS; k++) layouts[i].matrizAdj[j][k] = (j == k) ? 0 : INFINITO;
    }
    layouts[BLOCO_D].numRuas = 10;
    for (int i = 0; i < 9; i++) { layouts[BLOCO_D].matrizAdj[i][i+1] = 5; layouts[BLOCO_D].matrizAdj[i+1][i] = 5; }
    layouts[BLOCO_F].numRuas = 8;
    for (int i = 0; i < 7; i++) { layouts[BLOCO_F].matrizAdj[i][i+1] = 8; layouts[BLOCO_F].matrizAdj[i+1][i] = 8; }
    layouts[BLOCO_F].matrizAdj[1][5] = 10; layouts[BLOCO_F].matrizAdj[5][1] = 10;
    layouts[WMS].numRuas = 12;
    for (int i = 0; i < 11; i++) { layouts[WMS].matrizAdj[i][i+1] = (i%2==0)?4:6; layouts[WMS].matrizAdj[i+1][i] = (i%2==0)?4:6; }
    layouts[WMS].matrizAdj[0][11] = 20; layouts[WMS].matrizAdj[11][0] = 20;
    layouts[BLOCO_ACO].numRuas = 5;
    for (int i = 0; i < 4; i++) { layouts[BLOCO_ACO].matrizAdj[i][i+1] = 15; layouts[BLOCO_ACO].matrizAdj[i+1][i] = 15; }
    layouts[MOSTRUARIO].numRuas = 1; layouts[AREA_EXTERNA].numRuas = 1;
}

int calcularRotaRua(const LayoutLocal* layout, int ruaInicial, int ruaDestino, int* caminho, int* tamanhoCaminho) {
    ruaInicial--; ruaDestino--;
    int numRuas = layout->numRuas, dist[MAX_RUAS], vis[MAX_RUAS]={0}, ant[MAX_RUAS];
    for (int i = 0; i < numRuas; i++) { dist[i] = layout->matrizAdj[ruaInicial][i]; ant[i] = ruaInicial; }
    dist[ruaInicial] = 0; vis[ruaInicial] = 1;
    for (int c = 1; c < numRuas; c++) {
        int min_d = INFINITO, prox = -1;
        for (int i = 0; i < numRuas; i++) if (dist[i] < min_d && !vis[i]) { min_d = dist[i]; prox = i; }
        if (prox == -1) break;
        vis[prox] = 1;
        for (int i=0; i<numRuas; i++) if (!vis[i] && (min_d + layout->matrizAdj[prox][i] < dist[i])) { dist[i]=min_d+layout->matrizAdj[prox][i]; ant[i]=prox; }
    }
    if (dist[ruaDestino] == INFINITO) return 0;
    int tempPath[MAX_RUAS], count = 0;
    for (int at = ruaDestino; at != ruaInicial; at = ant[at]) tempPath[count++] = at;
    tempPath[count++] = ruaInicial;
    *tamanhoCaminho = count;
    for(int i = 0; i < count; i++) caminho[i] = tempPath[count - 1 - i] + 1;
    return 1;
}

void navegacaoInterativa(const LayoutLocal* layout, int* caminho, int tamanhoCaminho) {
    if (tamanhoCaminho <= 1) { printf("\nVoce ja esta na rua de destino!\n"); return; }
    printf("\n--- Iniciando Navegacao em '%s' ---\n", layout->nome);
    for (int i = 0; i < tamanhoCaminho; i++) {
        printf("\nVoce esta na ---> Rua %d\n", caminho[i]);
        if (i == tamanhoCaminho - 1) { printf("\n*** CHEGOU AO DESTINO! ***\n"); break; }
        printf("Pressione Enter para ir para a proxima rua (%d)...", caminho[i+1]);
        pressioneEnterParaContinuar();
    }
}

// ================= FUNÇÕES DO ESTOQUE =================

int obterIndiceLocal(const char* n) { for (int i = 0; i < NUM_LOCAIS; i++) if (strcmp(n, nomesLocais[i]) == 0) return i; return -1; }
int buscarIndiceProdutoPorAdm(estoque *d, int t, int a) { for (int i = 0; i < t; i++) if (d[i].adm == a) return i; return -1; }

void armazenarProduto(estoque *deposito, int i) {
    int opcaoLocal, opcaoRua;
    printf("\n--- Armazenamento do Produto ---\n");
    printf("1-WMS | 2-Area Externa | 3-Bloco D | 4-Bloco F | 5-Mostruario | 6-Bloco de Aco\n");
    opcaoLocal = lerInteiro("Escolha o local: ");
    int indiceLocal = opcaoLocal;
    if (opcaoLocal < 1 || opcaoLocal > 6) {
        indiceLocal = WMS;
        printf("Opcao invalida, definindo para WMS.\n");
    }
    strcpy(deposito[i].localArmazenamento, nomesLocais[indiceLocal]);
    int numRuasDisponiveis = layouts[indiceLocal].numRuas;
    if (numRuasDisponiveis <= 0) {
        printf("Este local nao possui ruas. Rua definida como 1.\n");
        deposito[i].ruaArmazenamento = 1;
    } else if (numRuasDisponiveis == 1) {
        printf("Este local possui apenas a Rua 1. Definido automaticamente.\n");
        deposito[i].ruaArmazenamento = 1;
    } else {
        do {
            char prompt[100];
            sprintf(prompt, "Digite a rua (1 a %d): ", numRuasDisponiveis);
            opcaoRua = lerInteiro(prompt);
            if (opcaoRua < 1 || opcaoRua > numRuasDisponiveis) printf("Rua invalida! Tente novamente.\n");
        } while (opcaoRua < 1 || opcaoRua > numRuasDisponiveis);
        deposito[i].ruaArmazenamento = opcaoRua;
    }
}

int cadastrarProduto(estoque *d, int i, int t) {
    printf("\n--- Cadastro do Produto %d ---\n", i + 1);
    int adm = lerInteiro("Digite o ADM do produto: ");
    
    // Validar ADM duplicado
    if (buscarIndiceProdutoPorAdm(d, i, adm) != -1) {
        printf("Erro: O ADM %d ja esta cadastrado no sistema!\n", adm);
        return 0; // Falha no cadastro
    }
    
    d[i].adm = adm;
    lerString("Digite o nome do produto: ", d[i].nomeProduto, 30);
    lerString("Digite a marca do produto: ", d[i].marcaProd, 15);
    d[i].codBarras = lerLong("Digite o codigo de barras do produto: ");
    d[i].codOriginal = lerLong("Digite o codigo original do produto: ");
    d[i].quantidade = lerInteiro("Informe a quantidade do produto: ");
    armazenarProduto(d, i);
    return 1; // Sucesso
}

void editarProduto(estoque* d, int t) {
    int adm = lerInteiro("Digite o ADM do produto que deseja editar: "), idx = buscarIndiceProdutoPorAdm(d, t, adm);
    if (idx == -1) { printf("Produto com ADM %d nao encontrado.\n", adm); return; }
    printf("\n--- Editando Produto: %s ---\n", d[idx].nomeProduto);
    printf("Deixe em branco e pressione Enter para manter o valor atual.\n\n");
    char b[100]; int novoInt;
    lerString("Novo nome: ", b, 30); if(strlen(b)>0) strcpy(d[idx].nomeProduto,b);
    lerString("Nova marca: ", b, 15); if(strlen(b)>0) strcpy(d[idx].marcaProd,b);
    printf("Nova quantidade (atual: %d): ", d[idx].quantidade);
    if(fgets(b,sizeof(b),stdin)!=NULL && sscanf(b,"%d",&novoInt)==1) d[idx].quantidade=novoInt;
    printf("Deseja alterar o local de armazenamento? (s/n): ");
    if(fgets(b,sizeof(b),stdin)!=NULL && (b[0]=='s'||b[0]=='S')) armazenarProduto(d,idx);
    printf("\nProduto atualizado com sucesso!\n");
}

void removerProduto(estoque **d, int *t) {
    if (*t==0){printf("Nao ha produtos para remover.\n");return;}
    int adm=lerInteiro("Digite o ADM do produto que deseja remover: "),idx=buscarIndiceProdutoPorAdm(*d,*t,adm);
    if(idx==-1){printf("Produto com ADM %d nao encontrado.\n", adm);return;}
    for(int i=idx;i<*t-1;i++) (*d)[i]=(*d)[i+1];
    (*t)--;
    if(*t>0){estoque* temp=realloc(*d,(*t)*sizeof(estoque)); if(temp!=NULL) *d=temp;} 
    else {free(*d);*d=NULL;}
    printf("Produto removido com sucesso!\n");
}

void relatorioArmazenagem(estoque *d, int t) {
    if (t==0) { printf("\nNenhum produto cadastrado no estoque.\n"); return; }
    printf("\n--- Relatorio de Produtos ---\n");
    printf("Total de produtos cadastrados: %d\n\n", t);
    
    // Tabela organizada
    printf("%-10s | %-20s | %-15s | %-10s | %-15s | %-5s\n", "ADM", "Nome", "Marca", "Quantidade", "Local", "Rua");
    printf("----------------------------------------------------------------------------------------\n");
    for (int i=0; i<t; i++) {
        printf("%-10d | %-20.20s | %-15.15s | %-10d | %-15.15s | %-5d\n", 
            d[i].adm, d[i].nomeProduto, d[i].marcaProd, d[i].quantidade, d[i].localArmazenamento, d[i].ruaArmazenamento);
    }
}

// ================= PERSISTÊNCIA E LOGÍSTICA =================

void salvarEstoque(estoque *d, int t) {
    FILE *a=fopen(ARQUIVO_ESTOQUE, "w"); if(!a){printf("Erro ao abrir arquivo!\n");return;}
    fprintf(a, "Total de Produtos: %d\n", t);
    for (int i=0; i<t; i++) {
        fprintf(a, "---\nADM: %d\nNome: %s\nMarca: %s\nCodigo de Barras: %ld\nCodigo Original: %ld\nQuantidade: %d\nLocal: %s\nRua: %d\n",
            d[i].adm, d[i].nomeProduto, d[i].marcaProd, d[i].codBarras, d[i].codOriginal, d[i].quantidade, d[i].localArmazenamento, d[i].ruaArmazenamento);
    }
    fclose(a); printf("Estoque salvo com sucesso!\n");
}

int carregarEstoque(estoque **d, int *t) {
    FILE *a = fopen(ARQUIVO_ESTOQUE, "r");
    if (!a) { if ((a = fopen(ARQUIVO_ESTOQUE, "w"))) { fprintf(a, "Total de Produtos: 0\n"); fclose(a); } return 0; }
    char linha[256]; fgets(linha, sizeof(linha), a); sscanf(linha, "Total de Produtos: %d", t);
    if (*t <= 0) { fclose(a); *t = 0; return 0; }
    *d = malloc(*t * sizeof(estoque)); if (!*d) { fclose(a); return 0; }
    int i = 0;
    while(i < *t && fgets(linha, sizeof(linha), a)) {
        if (strcmp(linha, "---\n") == 0) {
            (*d)[i].ruaArmazenamento = 1; // Padrão
            fgets(linha,sizeof(linha),a); sscanf(linha,"ADM: %d",&(*d)[i].adm);
            fgets(linha,sizeof(linha),a); sscanf(linha,"Nome: %[^\n]",(*d)[i].nomeProduto);
            fgets(linha,sizeof(linha),a); sscanf(linha,"Marca: %[^\n]",(*d)[i].marcaProd);
            fgets(linha,sizeof(linha),a); sscanf(linha,"Codigo de Barras: %ld",&(*d)[i].codBarras);
            fgets(linha,sizeof(linha),a); sscanf(linha,"Codigo Original: %ld",&(*d)[i].codOriginal);
            fgets(linha,sizeof(linha),a); sscanf(linha,"Quantidade: %d",&(*d)[i].quantidade);
            fgets(linha,sizeof(linha),a); sscanf(linha,"Local: %[^\n]",(*d)[i].localArmazenamento);
            if(fgets(linha,sizeof(linha),a)) sscanf(linha,"Rua: %d",&(*d)[i].ruaArmazenamento);
            i++;
        }
    }
    fclose(a); *t = i; printf("Estoque carregado! %d produtos encontrados.\n", *t); return *t;
}

// Funções de Fila e Pilha mantidas idênticas (compactadas por espaço)
void gerarBloqueio(estoque *d, int t, PilhaBloqueio *p) { int adm = lerInteiro("Informe o ADM: "), idx = buscarIndiceProdutoPorAdm(d, t, adm); if(idx!=-1){int q=lerInteiro("Qtd a bloquear: ");if(d[idx].quantidade>=q){d[idx].quantidade-=q; Bloqueio o={adm, q}; push(p, o); salvarEstoque(d,t); printf("Bloqueado! Estoque atual: %d\n",d[idx].quantidade);}else{printf("Qtd indisponivel.\n");}}else{printf("Nao encontrado!\n");} }
void reverterUltimoBloqueio(estoque *d, int t, PilhaBloqueio *p) { Bloqueio o; if(!pop(p,&o)){printf("Nenhum bloqueio para reverter.\n");return;} int idx=buscarIndiceProdutoPorAdm(d,t,o.adm); if(idx!=-1){d[idx].quantidade+=o.quantidadeBloqueada; salvarEstoque(d,t); printf("Revertido! Estoque: %d\n",d[idx].quantidade);} }
void processarFilaPedidos(estoque *d, int t, FilaPedidos *f) { if(f->contador==0){printf("\nFila vazia.\n");return;} printf("\n--- Processando Pedidos ---\n"); int proc=0; Pedido ped; while(desenfileirar(f,&ped)){int idx=buscarIndiceProdutoPorAdm(d,t,ped.adm);if(idx!=-1){if(d[idx].quantidade>=ped.quantidadeSolicitada){d[idx].quantidade-=ped.quantidadeSolicitada;printf("ADM %d atendido. Restam: %d\n",ped.adm, d[idx].quantidade);proc++;}else{printf("ADM %d: Estoque insuficiente.\n",ped.adm);}}else{printf("ADM %d: Nao encontrado.\n",ped.adm);}} if(proc>0)salvarEstoque(d,t); }
void limparEstoque(estoque **d, int *t) { if(*d!=NULL){free(*d);*d=NULL;} *t=0; FILE*a=fopen(ARQUIVO_ESTOQUE,"w"); if(a){fprintf(a,"Total de Produtos: 0\n");fclose(a);printf("\nEstoque limpo!\n");} }

// ================= NAVEGAÇÃO =================
void menuNavegacao(estoque *deposito, int totalProdutos) {
    int adm = lerInteiro("ADM do produto para coleta: ");
    int idxProd = buscarIndiceProdutoPorAdm(deposito, totalProdutos, adm);
    if (idxProd == -1) { printf("Produto nao encontrado!\n"); return; }

    printf("Produto '%s' esta em '%s', Rua %d.\n", deposito[idxProd].nomeProduto, deposito[idxProd].localArmazenamento, deposito[idxProd].ruaArmazenamento);
    printf("\nEm qual local voce esta agora?\n1-WMS|2-Area Ext|3-Bloco D|4-Bloco F|5-Mostruario|6-Bloco Aco\n");
    int idxLocalAtual = lerInteiro("Sua escolha: ");
    if (idxLocalAtual < 1 || idxLocalAtual > 6) { printf("Local invalido.\n"); return; }

    int idxLocalProduto = obterIndiceLocal(deposito[idxProd].localArmazenamento);
    if(idxLocalAtual != idxLocalProduto) { printf("Erro: Voce precisa estar no mesmo local do produto (%s).\n", deposito[idxProd].localArmazenamento); return; }

    const LayoutLocal* layout = &layouts[idxLocalAtual];
    if (layout->numRuas <= 1) { printf("Este local nao possui um sistema de ruas para roteamento.\n"); return; }

    int ruaDestino = deposito[idxProd].ruaArmazenamento;
    if (ruaDestino < 1 || ruaDestino > layout->numRuas) { printf("Erro: Dados de rua invalidos (Rua %d).\n", ruaDestino); return; }

    char prompt[100]; sprintf(prompt, "Em qual rua voce esta? (1 a %d): ", layout->numRuas);
    int ruaInicial = lerInteiro(prompt);
    if (ruaInicial < 1 || ruaInicial > layout->numRuas) { printf("Rua de partida invalida.\n"); return; }

    int caminho[MAX_RUAS], tamCaminho = 0;
    if(calcularRotaRua(layout, ruaInicial, ruaDestino, caminho, &tamCaminho)) {
        navegacaoInterativa(layout, caminho, tamCaminho);
    } else {
        printf("Nao foi possivel calcular uma rota da Rua %d para a Rua %d.\n", ruaInicial, ruaDestino);
    }
}

// ================= MAIN =================

int main() {
    estoque *deposito = NULL; int totalProdutos = 0, opcao;
    PilhaBloqueio pilhaBloqueios; inicializarPilha(&pilhaBloqueios);
    FilaPedidos filaDePedidos; inicializarFila(&filaDePedidos);
    inicializarMapasLocais();
    totalProdutos = carregarEstoque(&deposito, &totalProdutos);
    pressioneEnterParaContinuar();

    do {
        #if defined(_WIN32) || defined(_WIN64)
            system("cls");
        #else
            system("clear");
        #endif

        printf("\n============================================\n");
        printf("     SISTEMA DE GERENCIAMENTO DE ESTOQUE      ");
        printf("\n============================================\n");
        printf(" 1. Cadastrar Produto   | 7. Adicionar Pedido\n");
        printf(" 2. Editar Produto      | 8. Processar Fila\n");
        printf(" 3. Remover Produto     | 9. Navegar p/ Coleta\n");
        printf(" 4. Relatorio em Tabela | 10. Limpar Estoque\n");
        printf(" 5. Bloquear Estoque    | 11. Sair\n");
        printf(" 6. Reverter Bloqueio   |\n");
        printf("============================================\n");
        opcao = lerInteiro("Escolha uma opcao: ");

        switch (opcao) {
            case CADASTRAR: {
                int qtd = lerInteiro("\nQuantos produtos serao cadastrados? ");
                if (qtd > 0) {
                    estoque* temp = realloc(deposito, (totalProdutos + qtd) * sizeof(estoque));
                    if (temp) {
                        deposito = temp;
                        int cadastradosComSucesso = 0;
                        for(int i = 0; i < qtd; i++) {
                            if (cadastrarProduto(deposito, totalProdutos, totalProdutos + qtd)) {
                                totalProdutos++;
                                cadastradosComSucesso++;
                            } else {
                                printf("Cadastro abortado para este item devido a erro de validacao.\n");
                            }
                        }
                        if (cadastradosComSucesso > 0) salvarEstoque(deposito, totalProdutos);
                    } else { printf("Erro de memoria!\n"); }
                } 
                break;
            }
            case EDITAR: editarProduto(deposito, totalProdutos); salvarEstoque(deposito, totalProdutos); break;
            case REMOVER: removerProduto(&deposito, &totalProdutos); salvarEstoque(deposito, totalProdutos); break;
            case RELATORIO: relatorioArmazenagem(deposito, totalProdutos); break;
            case BLOQUEAR: gerarBloqueio(deposito, totalProdutos, &pilhaBloqueios); break;
            case REVERTER: reverterUltimoBloqueio(deposito, totalProdutos, &pilhaBloqueios); break;
            case ADD_PEDIDO: { Pedido p; p.adm=lerInteiro("ADM: ");p.quantidadeSolicitada=lerInteiro("Qtd: ");if(p.quantidadeSolicitada>0)enfileirar(&filaDePedidos, p); break; }
            case PROC_FILA: processarFilaPedidos(deposito, totalProdutos, &filaDePedidos); break;
            case NAVEGAR: menuNavegacao(deposito, totalProdutos); break;
            case LIMPAR: limparEstoque(&deposito, &totalProdutos); inicializarFila(&filaDePedidos); inicializarPilha(&pilhaBloqueios); break;
            case SAIR: printf("Saindo do sistema...\n"); break;
            default: printf("Opcao invalida!\n");
        }
        if (opcao != SAIR) pressioneEnterParaContinuar();
    } while (opcao != SAIR);
    
    if (deposito) free(deposito);
    return 0;
}