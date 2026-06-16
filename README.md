# b-tree_implementation-aed

Implementação de Árvore B — PPGCA DCM, Algoritmos e Estruturas de Dados.

## Visão geral

Uma Árvore B orientada a disco implementada em C++. Os nós são armazenados como
registros de tamanho fixo dentro de um arquivo binário (`tmp/btree.dat`), e a
árvore opera deslocando-se para offsets `registro * sizeof(Node)`, imitando o
I/O baseado em páginas do mundo real.

- **Linguagem de programação:** C++17
- **Descrição**: O idioma usado em comentários, descrições e no código é o português (pt-br).

## Armazenamento em disco (`tmp/btree.dat`)

O arquivo `btree.dat` **não é texto**: ele é um arquivo **binário** que a árvore
manipula diretamente, byte a byte, sem nenhuma serialização intermediária (nada
de JSON, CSV ou texto legível). Assim, estamos imitando o
I/O paginado de um SGBD real, onde cada nó é uma "página" de disco.

### Registros de tamanho fixo

Cada nó é o struct `BTreeNode<M>` (`src/node.h`), composto apenas por inteiros:

```cpp
template <int M>
struct BTreeNode {
    int numKeys;     // quantas chaves o nó guarda
    int K[M+1];      // chaves
    int A[M+1];      // ponteiros para filhos (ids de registro)
};
```

Como todos os campos são `int`, não há *padding*, e cada registro ocupa
exatamente `sizeof(BTreeNode<M>) = (2M + 3) * sizeof(int)` bytes — um tamanho
fixo conhecido em tempo de compilação. O arquivo inteiro é, portanto, um vetor
contíguo desses registros de tamanho fixo.

### Endereçamento direto por offset

Não existe índice nem tabela de localização: o **id do nó é o seu índice no
vetor**. Para acessar o nó `id`, a árvore calcula o deslocamento em bytes e
posiciona o ponteiro do arquivo diretamente ali (`src/btree.h`):

```cpp
file.seekp(id * sizeof(BTreeNode<M>), std::ios::beg);   // escrita
file.seekg(id * sizeof(BTreeNode<M>), std::ios::beg);   // leitura
```

### Leitura e escrita de bytes crus

O nó é transferido entre memória e disco como um *dump* direto do struct,
reinterpretando o seu endereço como uma sequência de bytes (`char*`):

```cpp
file.write((const char*)(&node), sizeof(BTreeNode<M>));  // memória → disco
file.read ((char*)(&node),       sizeof(BTreeNode<M>));  // disco → memória
```

O arquivo é aberto em modo binário (`std::ios::binary`), e cada escrita é
seguida de `file.flush()` para forçar a gravação. Logo, sim: **a manipulação é
binária e direta**, sem conversão de/para texto.

### Registro 0: cabeçalho, não um nó

O primeiro registro (id `0`) é reservado como **cabeçalho de metadados**, não é
um nó real da árvore. Ele reaproveita os campos do struct para guardar:

- `A[0]` → id do registro da **raiz** (`rootID`, recuperado no construtor ao
  reabrir um arquivo existente);
- `A[1]` → topo da **lista de registros livres** (ver abaixo).

### Reuso de registros (lista de livres)

Quando um nó é removido, seu registro não é apagado fisicamente: ele entra em uma
**lista encadeada de livres** ("lixeira"), cujo topo fica em `A[1]` do cabeçalho
e cujo encadeamento usa o campo `A[0]` de cada registro liberado. `allocateNode()`
primeiro tenta reciclar um registro dessa lista; só quando ela está vazia é que o
arquivo cresce, acrescentando um novo registro no fim (`seekp(0, std::ios::end)`).
Isso evita que o arquivo cresça indefinidamente conforme a árvore sofre inserções
e remoções.

### Contabilidade de I/O

Cada `readNode`/`writeNode` é contado e cronometrado pelo `DiskManager`
(`src/diskmanager.h`) — uma leitura/escrita por registro tocado, equivalente a
uma página de disco. São esses contadores (`reads`, `writes`, tempo de I/O,
tamanho do arquivo em bytes) que alimentam o CSV dos [experimentos](#experimentos).

> **Observação sobre portabilidade:** por ser um *dump* bruto do struct, o
> formato do `.dat` depende da plataforma (tamanho de `int`, *endianness*,
> alinhamento). O arquivo não é portável entre arquiteturas diferentes, e um
> arquivo gerado para uma ordem `M` não pode ser reaberto com outra ordem.

## Estrutura do projeto

```
.
├── src/                 # todos os fontes e headers C++ (main.cpp fica aqui)
├── tests/
│   ├── test_btree.cpp   # bateria de testes com dados de mock (main() próprio, compilado de forma independente)
│   └── fixtures/        # fixtures de texto, um por caso de teste (ver nomenclatura abaixo)
├── run_experiments.sh   # roda a bateria de experimentos e gera o CSV de resultados
├── analysis/            # notebook Jupyter de análise dos resultados dos experimentos
├── data/                # CSVs com os resultados dos experimentos
├── Makefile
└── tmp/                 # criado em tempo de execução; guarda btree.dat e os arquivos de dados dos testes
```

Os fixtures das operações que **alteram estado** são nomeados conforme o caso de
teste que os usa — `insert_<letra>_before.txt` / `insert_<letra>_after.txt` e
`remove_<letra>_before.txt` / `remove_<letra>_after.txt`, onde `<letra>` é a
letra do caso (A, B, C…). O `search` é somente leitura e não tem estado
antes/depois: todos os seus casos consultam a mesma árvore em um único
`search_tree.txt` compartilhado, então suas letras A–K rotulam as consultas
individuais, não arquivos. Veja a [tabela de testes](#casos-de-teste-e-cobertura)
para a lista completa.

## Compilar e rodar

Requer `g++` (C++17) e `make`.

```bash
make                  # compila main.exe
make run              # compila (se necessário) e roda
make run_experiments  # roda a bateria de experimentos (gera o CSV de resultados)
make test             # compila e roda a bateria de testes com dados de mock
make clean            # remove o binário, os objetos e tmp/
```

`make run` cria `tmp/` automaticamente.

O programa popula uma Árvore B de exemplo e entra em um prompt interativo; digite
um número para buscar, ou Ctrl-C para sair.

## Experimentos

`make run_experiments` executa `run_experiments.sh`, que recompila o projeto e
roda a bateria de experimentos variando a ordem da árvore (`M`), o número de
chaves inseridas e o modo de inserção (sequencial ou aleatório). Cada execução
chama o binário em modo experimento:

```bash
./main.exe --experiment <ordem_m> <num_chaves> <1_seq|0_rand>
```

As ordens, quantidades de chaves e modos varridos estão definidos no início de
`run_experiments.sh`. Os resultados são gravados em
`data/resultados_experimentos.csv`, com uma linha por execução e as colunas:

```
M,numKeys,tipo,reads,writes,avg_reads,avg_writes,tempo_total,tempo_cpu,tempo_io,bytes
```

A análise dos resultados é feita no notebook `analysis/analysis_sem_reuso.ipynb`.

## Testes

`make test` compila e roda `tests/test_btree.cpp`, que exercita `search()`,
`insert()` e `remove()`. A maioria dos casos são os exemplos resolvidos dos
slides da aula (`AED-03-Arvores-B.pdf`, ordem m=3); o restante são casos de
borda adicionados para cobrir caminhos que os slides não alcançam (empréstimo
do irmão da esquerda, remoção de chave interna, split em cascata, no-ops e ordem
m≠3). Todos os 27 casos passam.

### Casos de teste e cobertura

Os casos são identificados por letra dentro de cada suíte (A, B, C…). Para as
operações que alteram estado (`insert`, `remove`), cada caso tem seu próprio par
de fixtures autocontido compartilhando essa letra —
`<op>_<letra>_before.txt` / `<op>_<letra>_after.txt` (sem arquivos de cadeia
compartilhados). O `search` é somente leitura: todo caso consulta o mesmo
`search_tree.txt` compartilhado, então suas letras indexam consultas em vez de
arquivos. A coluna **Origem** indica se o caso é um exemplo resolvido dos slides
da aula (`AED-03-Arvores-B.pdf`) ou um caso de borda extra adicionado para
cobertura.

**`search()` — árvore B do slide 60** (somente leitura; todos os casos consultam `search_tree.txt`)

| Caso | Op / chave | Origem | O que cobre |
|------|------------|--------|-------------|
| A | search 30 | slides | chave presente na raiz |
| B | search 20 | slides | chave presente em um nó interno |
| C | search 40 | slides | chave presente em um nó interno |
| D | search 10 | slides | chave presente em uma folha |
| E | search 25 | slides | chave presente em uma folha |
| F | search 35 | slides | chave presente em uma folha |
| G | search 50 | slides | chave presente em uma folha |
| H | search 5 | slides | chave ausente, abaixo do mínimo |
| I | search 33 | slides | chave ausente, entre chaves existentes |
| J | search 99 | slides | chave ausente, acima do máximo |
| K | search 42 | extra | busca em árvore vazia (`rootID == 0`, montada inline, sem fixture) |

**`insert()`** (fixtures `insert_<letra>_before.txt` / `_after.txt`)

| Caso | Op / chave | Origem | O que cobre |
|------|------------|--------|-------------|
| A | insert 20 | extra | insere em folha com espaço sobrando (ordenado no lugar) |
| B | insert 10 | extra | inserir uma chave existente é no-op |
| C | insert 50 | extra | a primeira chave aloca a raiz |
| D | insert 30 | extra | folha-raiz cheia faz split, mediana promovida para uma nova raiz |
| E | insert 55 | slides | split da folha + mediana promovida para um pai existente |
| F | insert 28 | extra | split da folha estoura o pai, que também faz split → nova raiz (`split()` recursivo) |
| G | insert 25 | extra | split com ordem m=5 (mid = (M+1)/2), comprovando a generalidade da ordem |

**`remove()`** (fixtures `remove_<letra>_before.txt` / `_after.txt`)

| Caso | Op / chave | Origem | O que cobre |
|------|------------|--------|-------------|
| A | remove 58 | slides | remoção simples em folha, sem underflow |
| B | remove 65 | slides | underflow corrigido emprestando do irmão da **direita** |
| C | remove 55 | slides | underflow corrigido **fazendo merge** com um irmão (chave do pai puxada para baixo) |
| D | remove 40 | slides | merge em cascata que esvazia e **colapsa a raiz** |
| E | remove 50 | extra | chave em nó interno → substituída pelo **sucessor** em ordem (`getSuccessorKey`) |
| F | remove 80 | extra | underflow corrigido emprestando do irmão da **esquerda** (`borrowFromLeft`) |
| G | remove 42 | extra | remover a última chave colapsa a árvore para **vazia** |
| H | remove 99 | extra | remover uma chave ausente é no-op |
| I | remove 10 | extra | remover de uma árvore vazia é no-op |

Cada teste descreve uma árvore em um fixture de texto legível (`tests/fixtures/*.txt`):

```
root <id>
<id> <numKeys> <A[0]> <K[1]> <A[1]> ... <K[numKeys]> <A[numKeys]>
```

`BTree::loadFromFile()` constrói uma árvore diretamente a partir desse arquivo.
Um caso carrega a árvore *antes*, roda a operação e então compara o resultado
com a árvore *final* esperada. A comparação é **lógica**: percorre as duas
árvores a partir da raiz, comparando as chaves de cada nó e o formato da
subárvore, ignorando os ids físicos dos registros e os nós abandonados no disco.
Cada caso roda em um processo filho (fork), então uma regressão que cause crash é
reportada como um único `FAIL` em vez de abortar a suíte.
