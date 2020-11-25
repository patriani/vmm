#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// Campos da tabela de páginas
#define PT_FIELDS 6           // 4 campos na tabela

#define PT_FRAMEID 0          // Endereço da memória física
#define PT_MAPPED 1           // Endereço presente na tabela
#define PT_DIRTY 2            // Página dirty -> 1 para access_type == WRITE
#define PT_REFERENCE_BIT 3    // Bit de referencia
#define PT_REFERENCE_MODE 4   // Tipo de acesso, converter para char
#define PT_AGING_COUNTER 5    // Contador para aging

// Tipos de acesso
#define READ 'r'
#define WRITE 'w'

// Define a função que simula o algoritmo da política de subst.
typedef int (*eviction_f)(int8_t**, int, int, int, int, int);

typedef struct {
    char *name;
    void *function;
} paging_policy_t;

// Codifique as reposições a partir daqui!
// Cada método abaixo retorna uma página para ser trocada. Note também
// que cada algoritmo recebe:
// - A tabela de páginas
// - O tamanho da mesma
// - A última página acessada
// - A primeira modura acessada (para fifo)
// - O número de molduras
// - Se a última instrução gerou um ciclo de clock
//
// Adicione mais parâmetros caso ache necessário


/*
	int8_t** page_table: tabela de páginas
	int num_pages: tamanho da tabela 
	int prev_page: última acessada
	int fifo_frm: a primeira moldura acessada
	int num_frames: número de molduras
	int clock: "Se a última instrução gerou um ciclo de clock"

*/
int fifo(int8_t** page_table, int num_pages, int prev_page, int fifo_frm, int num_frames, int clock) {
    printf("entrou\n");
    for(int i = 0; i < num_pages; i++){
        if(page_table[i][PT_FRAMEID] == fifo_frm){
            printf("%d\n",i);
            printf("%d\n",page_table[i][PT_FRAMEID]);
            return i; // i representa o endereço virtual da página a ser subtituída
        }
    }
}

int second_chance(int8_t** page_table, int num_pages, int prev_page,
                  int fifo_frm, int num_frames, int clock) {
    return -1;
}

int nru(int8_t** page_table, int num_pages, int prev_page,
        int fifo_frm, int num_frames, int clock) {
    return -1;
}

int aging(int8_t** page_table, int num_pages, int prev_page,
          int fifo_frm, int num_frames, int clock) {
    return -1;
}

int random_page(int8_t** page_table, int num_pages, int prev_page,
                int fifo_frm, int num_frames, int clock) {
    int page = rand() % num_pages;
    while (page_table[page][PT_MAPPED] == 0) // Encontra página mapeada
        page = rand() % num_pages;
    return page;
}

// Simulador a partir daqui

int find_next_frame(int *physical_memory, int *num_free_frames,
                    int num_frames, int *prev_free) {
    if (*num_free_frames == 0) { // nenhum livre
        return -1;
    }

    // Procura por um frame livre de forma circula na memória.
    // Não é muito eficiente, mas fazer um hash em C seria mais custoso.
    do {
        *prev_free = (*prev_free + 1) % num_frames;
    } while (physical_memory[*prev_free] == 1); //enquanto ocupada

    return *prev_free;
}

int simulate(int8_t **page_table, int num_pages, int *prev_page, int *fifo_frm,
             int *physical_memory, int *num_free_frames, int num_frames,
             int *prev_free, int virt_addr, char access_type,
             eviction_f evict, int clock) {
    if (virt_addr >= num_pages || virt_addr < 0) {
        printf("Invalid access \n");
        exit(1);
    }

    if (page_table[virt_addr][PT_MAPPED] == 1) { // PT_MAPPED == 1 quando o endereço está na tabela
        page_table[virt_addr][PT_REFERENCE_BIT] = 1; // indica que página em questão foi acessada
        return 0; // Not Page Fault!
    }

    int next_frame_addr;
    if ((*num_free_frames) > 0) { // Ainda temos memória física livre!
        next_frame_addr = find_next_frame(physical_memory, num_free_frames,
                                          num_frames, prev_free); // retorna -1 se não tiver nenhum livre (*num_free_frames == 0)
        if (*fifo_frm == -1) // caso a variável de primeiro acesso à página estiver com valor default 
            *fifo_frm = next_frame_addr; // guarda endereço do ultimo primeiro inserido (mais antigo)
        *num_free_frames = *num_free_frames - 1;
    } else { // Precisamos liberar a memória (não há mais páginas livres e nenhuma das páginas virtuais corresponde à requisição)! -> chama o algoritmo
        assert(*num_free_frames == 0); // retorna erro caso *num_free_frames != 0
        int to_free = evict(page_table, num_pages, *prev_page, *fifo_frm,
                            num_frames, clock); // chama a função com o endereço em evict que retornará a página a ser liberada
        assert(to_free >= 0);
        assert(to_free < num_pages);
        assert(page_table[to_free][PT_MAPPED] != 0);

        next_frame_addr = page_table[to_free][PT_FRAMEID];
        *fifo_frm = (*fifo_frm + 1) % num_frames; // posição relativa ao número de frames -> frame com alocação mais antiga caminha em círculo (menos quando não há page fault - ele se mantém) 
        // Libera pagina antiga
        page_table[to_free][PT_FRAMEID] = -1;
        page_table[to_free][PT_MAPPED] = 0;
        page_table[to_free][PT_DIRTY] = 0;
        page_table[to_free][PT_REFERENCE_BIT] = 0;
        page_table[to_free][PT_REFERENCE_MODE] = 0;
        page_table[to_free][PT_AGING_COUNTER] = 0;
    }

    // Coloca endereço físico na tabela de páginas!
    int8_t *page_table_data = page_table[virt_addr];
    page_table_data[PT_FRAMEID] = next_frame_addr;
    page_table_data[PT_MAPPED] = 1;
    if (access_type == WRITE) {
        page_table_data[PT_DIRTY] = 1;
    }
    page_table_data[PT_REFERENCE_BIT] = 1;
    page_table_data[PT_REFERENCE_MODE] = (int8_t) access_type;
    *prev_page = virt_addr;

    if (clock == 1) {
        for (int i = 0; i < num_pages; i++)
            page_table[i][PT_REFERENCE_BIT] = 0;
    }

    return 1; // Page Fault!
}

void run(int8_t **page_table, int num_pages, int *prev_page, int *fifo_frm,
         int *physical_memory, int *num_free_frames, int num_frames,
         int *prev_free, eviction_f evict, int clock_freq) {
    int virt_addr;
    char access_type;
    int i = 0;
    int clock = 0;
    int faults = 0;
    while (scanf("%d", &virt_addr) == 1) { // enquanto puder ler um endereço virtual de anomaly
        getchar();
        scanf("%c", &access_type); // leitura de w do anomaly
        clock = ((i+1) % clock_freq) == 0;
        faults += simulate(page_table, num_pages, prev_page, fifo_frm,
                           physical_memory, num_free_frames, num_frames, prev_free,
                           virt_addr, access_type, evict, clock);
        i++;
    }
    printf("faults: %d\n", faults);
}

int parse(char *opt) {
    char* remainder;
    /*The  strtol()  function converts the initial part of the string in nptr
    to a long integer value according to the  given  base,  which  must  be
    between 2 and 36 inclusive, or be the special value 0.*/
    int return_val = strtol(opt, &remainder, 10);
    if (strcmp(remainder, opt) == 0) {
        printf("Error parsing: %s\n", opt);
        exit(1);
    }
    return return_val;
}

void read_header(int *num_pages, int *num_frames) {
    scanf("%d %d\n", num_pages, num_frames);
}


int main(int argc, char **argv) {
    if (argc < 3) { 
        printf("Usage %s <algorithm> <clock_freq>\n", argv[0]);
        exit(1);
    }

    char *algorithm = argv[1]; // especificação do algoritmo de substituição de página (random, por exemplo)
    int clock_freq = parse(argv[2]);
    
    // segundo anomaly.dat: num_pages = 10 & num_frames = 3
    int num_pages;
    int num_frames;
    read_header(&num_pages, &num_frames);

    // Aponta para cada função que realmente roda a política de parse
    paging_policy_t policies[] = {
            {"fifo", *fifo}, // nome == "fifo" & funcao == *fifo
            {"second_chance", *second_chance},
            {"nru", *nru},
            {"aging", *aging},
            {"random", *random_page}
    };
    
    //verifica qual o nome do algoritmo que bate com a entrada pelo terminal
    int n_policies = sizeof(policies) / sizeof(policies[0]);
    eviction_f evict = NULL;
    for (int i = 0; i < n_policies; i++) {
        if (strcmp(policies[i].name, algorithm) == 0) {
            evict = policies[i].function; // chama a função por seu endereço (funcao = *fifo)
            break;
        }
    }

    // se for null passou um de função nome não listado em policies[]
    if (evict == NULL) {
        printf("Please pass a valid paging algorithm.\n");
        exit(1);
    }

    // Aloca tabela (matriz) de páginas virtuais com tamanho baseado em num_pages == 10
    int8_t **page_table = (int8_t **) malloc(num_pages * sizeof(int8_t*));
    for (int i = 0; i < num_pages; i++) {
        // para cada campo do vetor page_table aloca PT_FIELDS (== 6) posições extra
        page_table[i] = (int8_t *) malloc(PT_FIELDS * sizeof(int8_t));
        // 
        page_table[i][PT_FRAMEID] = -1; // posição[i][0] == endereço da memória física
        page_table[i][PT_MAPPED] = 0;
        page_table[i][PT_DIRTY] = 0;
        page_table[i][PT_REFERENCE_BIT] = 0;
        page_table[i][PT_REFERENCE_MODE] = 0;
        page_table[i][PT_AGING_COUNTER] = 0; // posição[i][5] == contador para aging 
    }

    // Memória Real é apenas uma tabela de bits (na verdade uso ints) indicando
    // quais frames/molduras estão livre. 0 == livre!
    // Memória Real simulada a partir de um vetor de int com a quantidade de molduras lida na linha o de anomaly.dat
    int *physical_memory = (int *) malloc(num_frames * sizeof(int));
    for (int i = 0; i < num_frames; i++) {
        physical_memory[i] = 0;
    }
    
    int num_free_frames = num_frames; // frames livres inicializado com o numero total de molduras
    int prev_free = -1;
    int prev_page = -1;
    int fifo_frm = -1;

    // Roda o simulador
    srand(time(NULL)); //srand recebe uma seed baseada no clock da CPU para o retorno ser o mais randomico possível
    // page_table foi apenas inicializada
    run(page_table, num_pages, &prev_page, &fifo_frm, physical_memory,
        &num_free_frames, num_frames, &prev_free, evict, clock_freq);

    // Liberando os mallocs
    for (int i = 0; i < num_pages; i++) {
        free(page_table[i]);
    }
    free(page_table);
    free(physical_memory);
}
