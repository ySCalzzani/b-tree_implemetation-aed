# b-tree_implementation-aed

Implementação de Árvore B — PPGCA DCM, Algoritmos e Estruturas de Dados.

por Caio Uehara Martins
#TODO: Adicione os nomes de vocês aqui

## Visão geral

Uma Árvore B orientada a disco implementada em C++. Os nós são armazenados como
registros de tamanho fixo dentro de um arquivo binário (`tmp/btree.dat`), e a
árvore opera deslocando-se para offsets `registro * sizeof(Node)`, imitando o
I/O baseado em páginas do mundo real.

- **Linguagem de programação:** C++17
- **Descrição**: O idioma usado em comentários, descrições e no código é o português (pt-br).

## Estrutura do projeto

```
.
├── src/                 # todos os fontes e headers C++ (main.cpp fica aqui)
├── tests/
│   ├── test_btree.cpp   # bateria de testes com dados de mock (main() próprio, compilado de forma independente)
│   └── fixtures/        # fixtures de texto, um por caso de teste (ver nomenclatura abaixo)
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
make          # compila main.exe
make run      # compila (se necessário) e roda
make test     # compila e roda a bateria de testes com dados de mock
make clean    # remove o binário, os objetos e tmp/
```

`make run` cria `tmp/` automaticamente.

O programa popula uma Árvore B de exemplo e entra em um prompt interativo; digite
um número para buscar, ou Ctrl-C para sair.

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
