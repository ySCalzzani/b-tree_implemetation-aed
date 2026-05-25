#include <iostream>
#include <string>
#include <chrono>
#include <cstdlib>
#include <cstring>

#include "btree.h"

/* ----------------------------------------------------------------------------
 * runExperiment
 * Roda um experimento de inserção em uma árvore B, medindo o tempo gasto e as estatísticas de I/O.
 * * Parâmetros:
 * - numKeys     : Número total de chaves a inserir.
 * - sequential  : Se true, insere chaves em ordem crescente. Se false, insere chaves aleatórias.
 * * Saída:
 * Imprime uma linha CSV com os seguintes campos:
 * - M           : Ordem da árvore B.
 * - numKeys     : Número de chaves inseridas.
 * - tipo        : "Seq" para inserção sequencial, "Rand" para inserção aleatória.
 * - reads       : Número total de leituras de disco realizadas durante as inserções.
 * - writes      : Número total de escritas de disco realizadas durante as inserções.
 * - tempo       : Tempo total gasto para realizar as inserções, em segundos.
 * - bytes       : Tamanho final do arquivo de dados em bytes após as inserções.    
 * ------------------------------------------------------------------------- */
template <int M>
void runExperiment(int numKeys, bool sequential) {
    std::remove("tmp/experiment_btree.dat");
    BTree<M> tree("tmp/experiment_btree.dat");
    auto& disk = tree.getDiskManager();
    disk.resetCounters();

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numKeys; i++) {
        int key = sequential ? i : rand();
        tree.insert(key);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end_time - start_time;

    std::cout << M << "," 
              << numKeys << "," 
              << (sequential ? "Seq" : "Rand") << ","
              << disk.getReadCount() << "," 
              << disk.getWriteCount() << "," 
              << diff.count() << "," 
              << disk.getFileSize() << std::endl;
}

void exibirMenu() {
    std::cout << "\n=====================================\n";
    std::cout << "               ARVORE B                \n";
    std::cout << "=====================================\n";
    std::cout << "1. Inserir chave\n";
    std::cout << "2. Remover chave (A implementar)\n";
    std::cout << "3. Buscar chave (A implementar)\n";
    std::cout << "4. Imprimir arvore (A implementar)\n";
    std::cout << "5. Estatisticas de I/O\n";
    std::cout << "0. Voltar / Sair\n";
    std::cout << "Escolha uma opcao: ";
}

template <int M>
void iniciarMenu() {
    std::string filename = "tmp/interactive_btree_m" + std::to_string(M) + ".dat";
    BTree<M> tree(filename); 
    
    int opcao, chave;

    do {
        exibirMenu();
        std::cin >> opcao;

        switch (opcao) {
            case 1:
                std::cout << "Chave para inserir: ";
                std::cin >> chave;
                tree.insert(chave);
                std::cout << "Chave inserida!\n";
                break;
            case 2:
                std::cout << "Chave para remover: ";
                std::cin >> chave;
                tree.remove(chave);
                break;
            case 5:
                std::cout << "\n--- Estatisticas do Disco ---\n";
                std::cout << "Leituras (I/O): " << tree.getDiskManager().getReadCount() << "\n";
                std::cout << "Escritas (I/O): " << tree.getDiskManager().getWriteCount() << "\n";
                std::cout << "Tamanho do arq: " << tree.getDiskManager().getFileSize() << " bytes\n";
                break;
            case 0:
                std::cout << "Saindo...\n";
                break;
            default:
                if(opcao != 3 && opcao != 4) std::cout << "Opcao invalida!\n";
        }
    } while (opcao != 0);
}

/* ----------------------------------------------------------------------------
* modoInterativoMain
* Permite ao usuário escolher a ordem da árvore B (m) e inicia o menu correspondente.
* ------------------------------------------------------------------------- */
void modoInterativoMain() {
    int m_escolhido;
    std::cout << "Bem-vindo! Digite a ordem (m) desejada (ex: 3, 4, 100): ";
    
    if (!(std::cin >> m_escolhido)) return;

    switch (m_escolhido) {
        case 3:   iniciarMenu<3>();   break;
        case 4:   iniciarMenu<4>();   break;
        case 100: iniciarMenu<100>(); break;
        case 1000: iniciarMenu<1000>(); break;
        default:
            std::cout << "Erro: Ordem " << m_escolhido << " nao pre-compilada.\n";
            break;
    }
}

/* ----------------------------------------------------------------------------
* main
* Roda o modo interativo se nenhum argumento for passado. Se argumentos forem passados, roda o experimento.
* Uso:
* - Modo Interativo: ./btree
* - Modo Experimento: ./btree --experiment <ordem_m> <num_chaves> <1_seq|0_rand>
* ------------------------------------------------------------------------- */
int main(int argc, char* argv[]) {
    if (argc == 1) {
        modoInterativoMain();
        return 0;
    }

    if (argc == 5 && std::strcmp(argv[1], "--experiment") == 0) {
        int m = std::stoi(argv[2]);
        int numKeys = std::stoi(argv[3]);
        bool sequential = std::stoi(argv[4]) == 1;

        if (m == 3)        runExperiment<3>(numKeys, sequential);
        else if (m == 100) runExperiment<100>(numKeys, sequential);
        else if (m == 1000) runExperiment<1000>(numKeys, sequential);
        else std::cerr << "Erro: Ordem nao suportada.\n";
        
        return 0;
    }

    std::cerr << "Uso Interativo: ./btree\n";
    std::cerr << "Uso Experimento: ./btree --experiment <ordem_m> <num_chaves> <1_seq|0_rand>\n";
    return 1;
}