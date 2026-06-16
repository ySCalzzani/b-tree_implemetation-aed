#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>
#include <stdexcept>

#include "btree.h"
#include "display/display.h"

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
 * - avg_reads   : Média de leituras no disco por operação (reads / numKeys).
 * - avg_writes  : Média de escritas no disco por operação (writes / numKeys).
 * - tempo_total : Tempo total gasto para realizar as inserções, em segundos.
 * - tempo_cpu   : Tempo de processamento (CPU) isolado, em segundos.
 * - tempo_io    : Tempo de espera gasto exclusivamente com operações de I/O, em segundos.
 * - bytes       : Tamanho final do arquivo de dados em bytes após as inserções.
 * - altura      : Número de níveis da árvore após as inserções.
 * ------------------------------------------------------------------------- */
template <int M>
void runExperiment(int numKeys, bool sequential) {
    // Garante o diretório de saída: sem ele o fstream falharia em silêncio.
    std::filesystem::create_directories("tmp");
    // Começa sempre de um arquivo limpo para não contaminar as medições.
    std::remove("tmp/experiment_btree.dat");
    BTree<M> tree("tmp/experiment_btree.dat");

    // Zera os contadores de I/O antes de medir.
    auto& disk = tree.getDiskManager();
    disk.resetCounters();
    disk.resetIoTime();

    auto start_time = std::chrono::high_resolution_clock::now();

    // Insere numKeys chaves: sequenciais (i) ou aleatórias (rand()).
    for (int i = 0; i < numKeys; i++) {
        int key = sequential ? i : rand();
        tree.insert(key);
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    // Captura as métricas de I/O ANTES de medir a altura: getHeight() faz
    // leituras extras que não devem entrar nas contagens das inserções.
    long reads = disk.getReadCount();
    long writes = disk.getWriteCount();
    double io_time = disk.getIoTime();
    long bytes = disk.getFileSize();

    std::chrono::duration<double> diff_total = end_time - start_time;
    double total_time = diff_total.count();
    double cpu_time = total_time - io_time;  // tempo de CPU = total - I/O
    double avg_reads_per_op = static_cast<double>(reads) / numKeys;
    double avg_writes_per_op = static_cast<double>(writes) / numKeys;

    int altura = tree.getHeight();

    // Salvaguarda: arquivo de 0 bytes após inserir chaves indica falha de I/O.
    if (bytes == 0 && numKeys > 0) {
        std::cerr << "ERRO: arquivo de 0 bytes apos inserir " << numKeys
                  << " chaves (tmp/ ausente ou falha de I/O).\n";
        std::exit(2);
    }

    // Imprime uma linha CSV com todos os resultados.
    std::cout << M << ","
              << numKeys << ","
              << (sequential ? "Seq" : "Rand") << ","
              << reads << ","
              << writes << ","
              << avg_reads_per_op << ","
              << avg_writes_per_op << ","
              << total_time << ","
              << cpu_time << ","
              << io_time << ","
              << bytes << ","
              << altura << "\n";
}

/* ----------------------------------------------------------------------------
 * runReuseExperiment
 * Mede o impacto do REAPROVEITAMENTO de nós sobre a ocupação do arquivo, em três
 * fases: (1) insere numKeys chaves; (2) remove a primeira metade — gerando
 * underflow/merge que liberam nós para a lixeira; (3) insere numKeys/2 chaves
 * novas. Com reuso ativo, a fase 3 recicla os nós liberados e o arquivo quase
 * não cresce; com reuso desligado, allocateNode cresce o arquivo sempre. A
 * coluna 'bytes' final evidencia a economia.
 * Saída CSV: M,numKeys,tipo,reuso,reads,writes,avg_reads,avg_writes,tempo_total,bytes,altura
 * ------------------------------------------------------------------------- */
template <int M>
void runReuseExperiment(int numKeys, bool sequential, bool reuseEnabled) {
    std::filesystem::create_directories("tmp");
    std::remove("tmp/reuse_btree.dat");
    BTree<M> tree("tmp/reuse_btree.dat");
    tree.setReuse(reuseEnabled);

    auto& disk = tree.getDiskManager();
    disk.resetCounters();
    disk.resetIoTime();

    std::vector<int> keys;
    keys.reserve(numKeys);

    auto start_time = std::chrono::high_resolution_clock::now();

    // Fase 1: insere numKeys chaves (guardadas para poder removê-las depois).
    for (int i = 0; i < numKeys; i++) {
        int key = sequential ? i : rand();
        keys.push_back(key);
        tree.insert(key);
    }

    // Fase 2: remove a primeira metade — libera nós para a lixeira.
    int half = numKeys / 2;
    for (int i = 0; i < half; i++) {
        tree.remove(keys[i]);
    }

    // Fase 3: insere 'half' chaves novas (faixa acima das já inseridas).
    int base = sequential ? numKeys : 0;
    for (int i = 0; i < half; i++) {
        int key = sequential ? (base + i) : rand();
        tree.insert(key);
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    long reads = disk.getReadCount();
    long writes = disk.getWriteCount();
    long bytes = disk.getFileSize();
    int altura = tree.getHeight();

    std::chrono::duration<double> diff_total = end_time - start_time;
    double total_time = diff_total.count();
    int ops = numKeys + half + half;
    double avg_reads = static_cast<double>(reads) / ops;
    double avg_writes = static_cast<double>(writes) / ops;

    if (bytes == 0 && numKeys > 0) {
        std::cerr << "ERRO: arquivo de 0 bytes (tmp/ ausente ou falha de I/O).\n";
        std::exit(2);
    }

    std::cout << M << ","
              << numKeys << ","
              << (sequential ? "Seq" : "Rand") << ","
              << (reuseEnabled ? "com" : "sem") << ","
              << reads << ","
              << writes << ","
              << avg_reads << ","
              << avg_writes << ","
              << total_time << ","
              << bytes << ","
              << altura << "\n";
}

void exibirMenu() {
    std::cout << "=====================================\n";
    std::cout << "               ARVORE B                \n";
    std::cout << "=====================================\n";
    std::cout << "1. Inserir chave\n";
    std::cout << "2. Remover chave\n";
    std::cout << "3. Buscar chave\n";
    std::cout << "4. Imprimir arvore\n";
    std::cout << "5. Estatisticas de I/O\n";
    std::cout << "6. Dump bruto do arquivo .dat\n";
    std::cout << "0. Voltar / Sair\n";
    std::cout << "Escolha uma opcao: ";
}

/* ----------------------------------------------------------------------------
* renderTela
* Desenha a tela "dashboard": menu fixo no topo e, abaixo, uma regiao com o
* resultado da ultima acao. O cursor e devolvido para o prompt do menu (via
* ANSI \033[s salvar / \033[u restaurar), de modo que o usuario digita logo
* abaixo do menu enquanto o resultado permanece visivel na regiao de baixo.
* ------------------------------------------------------------------------- */
void renderTela(const std::string& resultado) {
    clearScreen();
    exibirMenu();
    std::cout << "\033[s";  // salva a posicao do cursor (no prompt do menu)

    std::cout << "\n\n+============== RESULTADO ==============+\n";
    std::cout << (resultado.empty() ? "(nenhuma acao ainda)\n" : resultado);
    std::cout << "+======================================+\n";

    std::cout << "\033[u";  // devolve o cursor para o prompt
    std::cout.flush();
}

/* ----------------------------------------------------------------------------
* lerChave
* Redesenha o menu limpo, ecoa a opcao escolhida e mostra um sub-prompt para
* o usuario digitar a chave, sem sujar a regiao de resultado.
* ------------------------------------------------------------------------- */
int lerChave(int opcao, const std::string& label) {
    clearScreen();
    exibirMenu();
    std::cout << opcao << "\n\n" << label;
    int v;
    std::cin >> v;
    return v;
}

// Captura as duas views da arvore (indentada + por nivel) numa string,
// redirecionando temporariamente o std::cout das funcoes de display.
template <int M>
std::string capturarArvore(BTree<M>& tree) {
    std::ostringstream oss;
    std::streambuf* antigo = std::cout.rdbuf(oss.rdbuf());

    std::cout << "--- Arvore (indentada) ---\n";
    displayTree(tree);
    std::cout << "\n--- Arvore (por nivel) ---\n";
    displayLevelOrder(tree);

    std::cout.rdbuf(antigo);
    return oss.str();
}

template <int M>
std::string capturarBusca(BTree<M>& tree, int chave) {
    std::ostringstream oss;
    std::streambuf* antigo = std::cout.rdbuf(oss.rdbuf());
    displaySearchResult(tree, chave);
    std::cout.rdbuf(antigo);
    return oss.str();
}

template <int M>
std::string capturarDump(BTree<M>& tree) {
    std::ostringstream oss;
    std::streambuf* antigo = std::cout.rdbuf(oss.rdbuf());
    dumpFile(tree);
    std::cout.rdbuf(antigo);
    return oss.str();
}

template <int M>
std::string capturarStats(BTree<M>& tree) {
    std::ostringstream oss;
    oss << "--- Estatisticas do Disco ---\n";
    oss << "Leituras (I/O): " << tree.getDiskManager().getReadCount() << "\n";
    oss << "Escritas (I/O): " << tree.getDiskManager().getWriteCount() << "\n";
    oss << "Tamanho do arq: " << tree.getDiskManager().getFileSize() << " bytes\n";
    return oss.str();
}

// Loop principal do modo interativo para uma árvore de ordem M: lê a opção do
// menu, executa a operação e guarda o texto do resultado para a próxima tela.
template <int M>
void iniciarMenu() {
    // Cada ordem M usa seu próprio arquivo .dat persistente.
    std::string filename = "tmp/interactive_btree_m" + std::to_string(M) + ".dat";
    BTree<M> tree(filename);

    int opcao, chave;
    std::string resultado;

    do {
        renderTela(resultado);
        std::cin >> opcao;

        switch (opcao) {
            case 1:
                chave = lerChave(1, "Chave para inserir: ");
                tree.insert(chave);
                resultado = "Chave " + std::to_string(chave) + " inserida.\n\n"
                          + capturarArvore(tree);
                break;
            case 2:
                chave = lerChave(2, "Chave para remover: ");
                tree.remove(chave);
                resultado = "Chave " + std::to_string(chave) + " removida.\n\n"
                          + capturarArvore(tree);
                break;
            case 3:
                chave = lerChave(3, "Chave para buscar: ");
                resultado = capturarBusca(tree, chave) + "\n" + capturarArvore(tree);
                break;
            case 4:
                resultado = capturarArvore(tree);
                break;
            case 5:
                resultado = capturarStats(tree);
                break;
            case 6:
                resultado = capturarDump(tree);
                break;
            case 0:
                clearScreen();
                std::cout << "Saindo...\n";
                break;
            default:
                resultado = "Opcao invalida!";
        }
    } while (opcao != 0);
}

/* ----------------------------------------------------------------------------
* modoInterativoMain
* Permite ao usuário escolher a ordem da árvore B (m) e inicia o menu correspondente.
* ------------------------------------------------------------------------- */
void modoInterativoMain() {
    int m_escolhido;
    std::cout << "Bem-vindo! Digite a ordem (m) desejada (ex: 3, 4, 5, 10, 50, 100, 500, 1000): ";
    
    if (!(std::cin >> m_escolhido)) return;

    switch (m_escolhido) {
        case 3:    iniciarMenu<3>();    break;
        case 4:    iniciarMenu<4>();    break;
        case 5:    iniciarMenu<5>();    break;
        case 10:   iniciarMenu<10>();   break;
        case 50:   iniciarMenu<50>();   break;
        case 100:  iniciarMenu<100>();  break;
        case 500:  iniciarMenu<500>();  break;
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
* - Modo Interativo: ./main.exe
* - Modo Experimento: ./main.exe --experiment <ordem_m> <num_chaves> <1_seq|0_rand>
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

        try {
            if (m == 3)        runExperiment<3>(numKeys, sequential);
            else if (m == 4)   runExperiment<4>(numKeys, sequential);
            else if (m == 5)   runExperiment<5>(numKeys, sequential);
            else if (m == 10)  runExperiment<10>(numKeys, sequential);
            else if (m == 50)  runExperiment<50>(numKeys, sequential);
            else if (m == 100) runExperiment<100>(numKeys, sequential);
            else if (m == 500) runExperiment<500>(numKeys, sequential);
            else if (m == 1000) runExperiment<1000>(numKeys, sequential);
            else { std::cerr << "Erro: Ordem nao suportada.\n"; return 1; }
        } catch (const std::exception& e) {
            std::cerr << "Falha no experimento: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    // Experimento de reaproveitamento: insere, remove metade e reinsere; o último
    // argumento liga (1) ou desliga (0) o reuso de nós para comparar a ocupação.
    if (argc == 6 && std::strcmp(argv[1], "--reuse-experiment") == 0) {
        int m = std::stoi(argv[2]);
        int numKeys = std::stoi(argv[3]);
        bool sequential = std::stoi(argv[4]) == 1;
        bool reuse = std::stoi(argv[5]) == 1;

        try {
            if (m == 3)        runReuseExperiment<3>(numKeys, sequential, reuse);
            else if (m == 4)   runReuseExperiment<4>(numKeys, sequential, reuse);
            else if (m == 5)   runReuseExperiment<5>(numKeys, sequential, reuse);
            else if (m == 10)  runReuseExperiment<10>(numKeys, sequential, reuse);
            else if (m == 50)  runReuseExperiment<50>(numKeys, sequential, reuse);
            else if (m == 100) runReuseExperiment<100>(numKeys, sequential, reuse);
            else if (m == 500) runReuseExperiment<500>(numKeys, sequential, reuse);
            else if (m == 1000) runReuseExperiment<1000>(numKeys, sequential, reuse);
            else { std::cerr << "Erro: Ordem nao suportada.\n"; return 1; }
        } catch (const std::exception& e) {
            std::cerr << "Falha no experimento de reuso: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    std::cerr << "Uso Interativo: ./main.exe\n";
    std::cerr << "Uso Experimento: ./main.exe --experiment <ordem_m> <num_chaves> <1_seq|0_rand>\n";
    std::cerr << "Uso Reuso:       ./main.exe --reuse-experiment <ordem_m> <num_chaves> <1_seq|0_rand> <1_com|0_sem>\n";
    return 1;
}