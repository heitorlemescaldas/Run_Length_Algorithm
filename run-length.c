#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Estrutura para armazenar informações da imagem PGM.
typedef struct {
    int largura, altura, maxval;
    unsigned char *dados;
} ImagemPGM;

FILE *abrirArquivo(const char *nomeArquivo, const char *modo) {
    FILE *arquivo = fopen(nomeArquivo, modo);
    if (!arquivo) {
        perror("Erro ao abrir o arquivo");
        return NULL;
    }
    return arquivo;
}

// Função para ler imagem PGM do arquivo.
ImagemPGM *lerPGM(const char *nomeArquivo) {
    FILE *arquivo = abrirArquivo(nomeArquivo, "r");


    ImagemPGM *imagem = malloc(sizeof(ImagemPGM));
    if (!imagem) {
        perror("Erro ao alocar memória para a imagem");
        fclose(arquivo);
        return NULL;
    }

    char linha[1024];
    if (!fgets(linha, sizeof(linha), arquivo)) { // Verifica leitura do tipo do arquivo PGM (P2)
        perror("Erro ao ler o tipo do arquivo PGM");
        free(imagem);
        fclose(arquivo);
        return NULL;
    }

    // Ignora comentários.
    do {
        if (!fgets(linha, sizeof(linha), arquivo)) {
            perror("Erro ao ler o cabeçalho da imagem");
            free(imagem);
            fclose(arquivo);
            return NULL;
        }
    } while (linha[0] == '#');

    if (sscanf(linha, "%d %d", &imagem->largura, &imagem->altura) != 2) {
        perror("Erro ao ler dimensões da imagem");
        free(imagem);
        fclose(arquivo);
        return NULL;
    }

    if (fscanf(arquivo, "%d", &imagem->maxval) != 1) {
        perror("Erro ao ler valor máximo de cor");
        free(imagem);
        fclose(arquivo);
        return NULL;
    }

    imagem->dados = malloc(imagem->largura * imagem->altura);
    if (!imagem->dados) {
        perror("Erro ao alocar memória para os dados da imagem");
        free(imagem);
        fclose(arquivo);
        return NULL;
    }

    for (int i = 0; i < imagem->largura * imagem->altura; i++) {
        int pixel;
        if (fscanf(arquivo, "%d", &pixel) != 1) {
            perror("Erro ao ler os valores dos pixels");
            free(imagem->dados);
            free(imagem);
            fclose(arquivo);
            return NULL;
        }
        imagem->dados[i] = (unsigned char)pixel;
    }

    fclose(arquivo);
    return imagem;
}

// Função para escrever imagem PGM no arquivo.
void escreverPGM(const char *nomeArquivo, const ImagemPGM *imagem)
{
    // Abre arquivo para escrita.
    FILE *arquivo = abrirArquivo(nomeArquivo, "w");


    // Escreve o cabeçalho da imagem PGM.
    fprintf(arquivo, "P2\n%d %d\n%d\n", imagem->largura, imagem->altura, imagem->maxval);

    // Escreve os dados dos pixels.
    for (int y = 0; y < imagem->altura; y++) {
        for (int x = 0; x < imagem->largura; x++) {
            int idx = y * imagem->largura + x;
            fprintf(arquivo, "%d ", imagem->dados[idx]);
        }
        fprintf(arquivo, "\n");  // Nova linha após escrever uma linha completa.
    }

    // Fecha o arquivo.
    fclose(arquivo);
}

// Função para compactar imagem PGM usando RLE.
void compactarRunLength(const ImagemPGM *imagem, const char *nomeArquivo) {
    // Abre arquivo para escrita.
    FILE *arquivo = abrirArquivo(nomeArquivo, "w");


    // Escreve cabeçalho para arquivo compactado.
    fprintf(arquivo, "P8\n%d %d\n%d\n", imagem->largura, imagem->altura, imagem->maxval);

    // Processo de compactação RLE.
    for (int y = 0; y < imagem->altura; y++) {
        int x = 0;
        while (x < imagem->largura) {
            int idx = y * imagem->largura + x; // Índice do pixel atual.
            unsigned char valor = imagem->dados[idx];
            int count = 1;
            // Conta quantos pixels consecutivos têm o mesmo valor.
            while (x + count < imagem->largura && imagem->dados[idx + count] == valor) {
                count++;
            }

            // Se a sequência tem 4 ou mais elementos, compacta. Caso contrário, escreve normalmente.
            if (count >= 4) {
                fprintf(arquivo, "@ %d %d ", valor, count);
            } else {
                for (int i = 0; i < count; i++) {
                    fprintf(arquivo, "%d ", valor);
                }
            }
            x += count;
        }
        fprintf(arquivo, "\n");
    }

    // Fecha o arquivo.
    fclose(arquivo);
}

// Função para descompactar imagem PGM compactada com RLE.
void descompactarRunLength(const char* nomeArquivoEntrada, const char* nomeArquivoSaida) {
    // Abre arquivo para leitura.
    FILE* arquivoEntrada = abrirArquivo(nomeArquivoEntrada, "r");


    // Lê cabeçalho do arquivo compactado.
    char formato[3];
    int largura, altura, maxval;
    fscanf(arquivoEntrada, "%s %d %d %d ", formato, &largura, &altura, &maxval);

    // Aloca memória para imagem descompactada.
    ImagemPGM imagem;
    imagem.largura = largura;
    imagem.altura = altura;
    imagem.maxval = maxval;
    imagem.dados = (unsigned char*)malloc(largura * altura);

    int idx = 0, valor, count;
    char ch;
    // Processo de descompactação RLE.
    while (idx < largura * altura && fscanf(arquivoEntrada, " %c", &ch) != EOF) {
        if (ch == '@') { // Indica sequência compactada.
            fscanf(arquivoEntrada, " %d %d", &valor, &count);
        } else {
            ungetc(ch, arquivoEntrada);
            fscanf(arquivoEntrada, "%d", &valor);
            count = 1;
        }

        for (int i = 0; i < count && idx < largura * altura; i++) {
            imagem.dados[idx++] = valor;
        }
    }

    // Fecha arquivo de entrada.
    fclose(arquivoEntrada);

    // Abre arquivo para escrita da imagem descompactada.
    FILE* arquivoSaida = abrirArquivo(nomeArquivoSaida, "w");
    if (!arquivoSaida) {
        free(imagem.dados); // Libera a memória alocada para os dados da imagem em caso de falha ao abrir o arquivo.
        return;
    }

    // Escreve o cabeçalho da imagem PGM descompactada no arquivo de saída.
    fprintf(arquivoSaida, "P2\n%d %d\n%d\n", largura, altura, maxval);

    // Escreve os dados dos pixels da imagem descompactada.
    for (int i = 0; i < largura * altura; i++) {
        fprintf(arquivoSaida, "%d", imagem.dados[i]);
        if ((i + 1) % largura == 0) {
            fprintf(arquivoSaida, "\n"); // Insere uma nova linha após escrever todos os pixels de uma linha.
        } else {
            fprintf(arquivoSaida, " "); // Insere espaço entre os valores dos pixels.
        }
    }

    // Fecha o arquivo de saída e libera a memória alocada para os dados da imagem.
    fclose(arquivoSaida);
    free(imagem.dados);
}

// Função principal que determina se o programa deve compactar ou descompactar baseado na extensão do arquivo.
int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Uso: %s <arquivo de entrada> <arquivo de saida>\n", argv[0]);
        return 1;
    }
    // Caminhos dos arquivos de entrada e saída definidos diretamente no código.
    char *entrada = argv[1];
    char *saida = argv[2];

    // Determina o modo com base na extensão do arquivo de entrada.
    char *extensao = strrchr(entrada, '.');
    if (extensao != NULL)
    {
        if (strcmp(extensao, ".pgm") == 0)
        {
            // Modo de compactação: lê a imagem PGM e a compacta usando RLE.
            ImagemPGM *imagem = lerPGM(entrada);
            if (imagem != NULL)
            {
                compactarRunLength(imagem, saida);
                free(imagem->dados); // Libera memória alocada para os dados da imagem.
                free(imagem); // Libera memória alocada para a estrutura da imagem.
            }
        }
        else if (strcmp(extensao, ".pgmc") == 0)
        {
            // Modo de descompactação: lê o arquivo compactado e cria uma imagem PGM descompactada.
            descompactarRunLength(entrada, saida);
        }
        else
        {
            // Erro caso a extensão do arquivo não seja suportada.
            printf("Extensão de arquivo inválida. Use '.pgm' para compactar ou '.pgmc' para descompactar.\n");
            return 1; // Encerra o programa com código de erro.
        }
    }
    else
    {
        // Erro caso não seja possível determinar a extensão do arquivo.
        printf("Não foi possível determinar o modo a partir do arquivo de entrada. Certifique-se de que a extensão seja '.pgm' ou '.pgmc'.\n");
        return 1; // Encerra o programa com código de erro.
    }

    return 0; // Encerra o programa com sucesso.
}
