#!/bin/bash

# Script para executar todos os testes da pasta testes/
# e salvar os outputs em outputs/

# Cores para output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Diretórios
TESTES_DIR="testes"
OUTPUT_DIR="outputs"
EXECUTAVEL="./pc"

# Criar diretório de outputs se não existir
mkdir -p "$OUTPUT_DIR"

# Verificar se o executável existe
if [ ! -f "$EXECUTAVEL" ]; then
    echo -e "${RED}ERRO: Executável '$EXECUTAVEL' não encontrado!${NC}"
    echo "Compilando..."
    gcc -o pc pc.c -pthread -O2 -Wall -Wextra
    if [ $? -ne 0 ]; then
        echo -e "${RED}ERRO: Falha na compilação!${NC}"
        exit 1
    fi
    echo -e "${GREEN}Compilação concluída!${NC}"
fi

# Verificar se a pasta de testes existe
if [ ! -d "$TESTES_DIR" ]; then
    echo -e "${RED}ERRO: Diretório '$TESTES_DIR' não encontrado!${NC}"
    exit 1
fi

# Contador de testes
total=0
sucessos=0
falhas=0

echo "=================================="
echo "  EXECUTANDO TESTES"
echo "=================================="
echo ""

# Loop através de todos os arquivos .txt na pasta testes
for teste in "$TESTES_DIR"/*.txt; do
    # Pegar apenas o nome do arquivo sem o caminho
    nome_teste=$(basename "$teste")
    # Remover extensão .txt
    nome_base="${nome_teste%.txt}"
    # Nome do arquivo de output
    output_file="$OUTPUT_DIR/output_${nome_base}.txt"
    
    total=$((total + 1))
    
    echo -n "Executando: ${YELLOW}$nome_teste${NC} ... "
    
    # Executar teste e capturar output e stderr
    if timeout 60 "$EXECUTAVEL" < "$teste" > "$output_file" 2>&1; then
        echo -e "${GREEN}✓ SUCESSO${NC}"
        sucessos=$((sucessos + 1))
    else
        exit_code=$?
        if [ $exit_code -eq 124 ]; then
            echo -e "${RED}✗ TIMEOUT (>60s)${NC}"
        else
            echo -e "${RED}✗ FALHOU (exit code: $exit_code)${NC}"
        fi
        falhas=$((falhas + 1))
    fi
done

echo ""
echo "=================================="
echo "  RESUMO"
echo "=================================="
echo "Total de testes:  $total"
echo -e "Sucessos:         ${GREEN}$sucessos${NC}"
echo -e "Falhas:           ${RED}$falhas${NC}"
echo ""
echo "Outputs salvos em: $OUTPUT_DIR/"
echo "=================================="

# Exit code baseado em falhas
if [ $falhas -eq 0 ]; then
    exit 0
else
    exit 1
fi
