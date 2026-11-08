# O que falta

Conforme a rede neural (no outro projeto) for pedindo.

- [ ] **Valor faltando**: hoje célula vazia é erro. Opções: preencher com a média/moda do `fit`, ou uma coluna extra "faltou".
- [ ] **Normalização** das colunas numéricas (média e desvio, ou mín e máx), guardada no `SCHEMA` para aplicar igual no treino e no teste.
- [ ] **Separar entrada e alvo**: escolher colunas para `X` e para `y` e gerar dois tensores com o mesmo `SCHEMA`.
- [ ] **Agrupar linhas em amostras** (janela deslizante, ou por uma coluna-chave), para batch dentro de um CSV só.
- [ ] **Salvar e carregar o `SCHEMA`**, para codificar dados novos sem refazer o `fit`.
- [ ] **CSV grande**: hoje o arquivo inteiro vai para a memória como texto e depois como células.

## Feito

- [x] Leitura RFC 4180 com posição do erro (`POSITION`).
- [x] `fit` com tipo inferido ou forçado, one-hot em ordem alfabética.
- [x] `encode` em `HOST::TENSOR [batch, linhas, colunas]` e `upload` para `DEVICE::TENSOR3`.
- [x] Testes na GPU e emulados (com ASan/UBSan).
