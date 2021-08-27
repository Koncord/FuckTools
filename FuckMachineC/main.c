#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void *memrchr(const void *s, int c, size_t n) {
    if (n > 0) {
        const char *p = (const char *) s;
        const char *q = p + n;
        while (1) {
            if (--q < p || q[0] == c) break;
            if (--q < p || q[0] == c) break;
            if (--q < p || q[0] == c) break;
            if (--q < p || q[0] == c) break;
        }
        if (q >= p) return (void *) q;
    }
    return NULL;
}

enum VMOpcode {
    VMOPCODE_INVALID = 0,
    VMOPCODE_INC,
    VMOPCODE_SUB,
    VMOPCODE_INCWIN,
    VMOPCODE_DECWIN,
    VMOPCODE_OUTCHAR,
    VMOPCODE_INCHAR,
    VMOPCODE_LOOPBEGIN,
    VMOPCODE_LOOPEND,
    VMOPCODE_SET,
    VMOPCODE_COPY,
    VMOPCODE_MUL,
    VMOPCODE_SCAN,
    VMOPCODE_MOV,
    VMOPCODE_MEMSET
};

typedef short HalfType;
typedef int ArgType;
typedef char CellType;
typedef char VMOpcode;
static const int maxCells = 30000; // classic BF

#pragma pack(push, 1)
typedef struct {
    VMOpcode opcode;
    union Arg {
        ArgType full;
        struct {
            HalfType offset;
            CellType arg2;
            CellType arg;
        } half;
    } arg;
} Instruction;
#pragma pack(pop)

static void error(char const *msg) {
    fprintf(stderr, "fuckmachine: %s\nexecution interrupted.\n", msg);
    exit(1);
}

static Instruction *readFile(char const *fname, int *codeLen) {
    FILE *file = fopen(fname, "rb");
    if (file == NULL)
        error("fatal error: file not found");
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    int opCnt = 0;
    fread(&opCnt, sizeof(int), 1, file);

    Instruction *buf = (Instruction *) malloc(sizeof(Instruction) * opCnt);
    int i = 0;
    for(; fread(&buf[i].opcode, sizeof(VMOpcode), 1, file) == 1;  ++i) {
        switch(buf[i].opcode) {
            case VMOPCODE_INC:
            case VMOPCODE_SUB:
            case VMOPCODE_OUTCHAR:
            case VMOPCODE_INCHAR:
            case VMOPCODE_SET:
            case VMOPCODE_COPY:
            case VMOPCODE_MUL:
            case VMOPCODE_MOV:
                if (fread(&buf[i].arg.half.offset, sizeof(HalfType), 1, file) != 1)
                    error("Reading error");
                if (fread(&buf[i].arg.half.arg, sizeof(CellType), 1, file) != 1)
                    error("Reading error");
                break;
            case VMOPCODE_INCWIN:
            case VMOPCODE_DECWIN:
            case VMOPCODE_LOOPBEGIN:
            case VMOPCODE_LOOPEND:
            case VMOPCODE_SCAN:
            case VMOPCODE_MEMSET:
                if (fread(&buf[i].arg.full, sizeof(ArgType), 1, file) != 1)
                    error("Reading error");
                break;
            default:
                error("Reading error");
        }
    }

    if (i != opCnt) {
        error("Read instructions does not match");
    }
    *codeLen = opCnt;
    fclose(file);
    return buf;
}

typedef struct {
    CellType *cells;
    ArgType currentCellN;
    ArgType currentCodeN;
    CellType *currentCell;
    Instruction *code;
    int codeSize;
} Machine;

static Machine *Machine_Create(char const *fname) {
    Machine *this = calloc(1, sizeof(Machine));
    this->cells = calloc(maxCells, sizeof(CellType));
    this->currentCell = &this->cells[0];
    this->code = readFile(fname, &this->codeSize);
    return this;
}

static void Machine_Free(Machine *this) {
    free(this->cells);
    free(this->code);
    free(this);
}

static Instruction *Machine_GetCodeCell(Machine *this) {
    if (this->currentCodeN >= this->codeSize)
        return NULL;
    return &this->code[this->currentCodeN++];
}

static void Machine_JmpCode(Machine *this, ArgType n) {
    this->currentCodeN = n;
    if (this->currentCodeN > this->codeSize)
        this->currentCodeN = 0;
    else if (this->currentCodeN < 0)
        this->currentCodeN = (ArgType) (this->codeSize);
}

static void Machine_MoveWindow(Machine *this, ArgType n) {
    this->currentCellN += n;
    this->currentCell = &this->cells[this->currentCellN];
}

int main(int argc, char *argv[]) {
    if (argc < 2)
        error("no input file");

    Machine *machine = Machine_Create(argv[1]);

    int ch;
    char *ptr;
    ArgType v;
    for (;;) {
        Instruction *cellCode = Machine_GetCodeCell(machine);
        if (cellCode == NULL) break;
        switch (cellCode->opcode) {
            case VMOPCODE_INC:
                *(machine->currentCell + cellCode->arg.half.offset) += cellCode->arg.half.arg;
                break;
            case VMOPCODE_INCWIN:
                Machine_MoveWindow(machine, cellCode->arg.full);
                break;
            case VMOPCODE_OUTCHAR:
                if (cellCode->arg.half.arg == -1)
                    putchar(*machine->currentCell);
                else
                    putchar(*(machine->currentCell + cellCode->arg.half.offset));
                break;
            case VMOPCODE_INCHAR:
                if ((ch = getchar()) == EOF)
                    exit(0);
                if (cellCode->arg.half.arg == -1)
                    *machine->currentCell = (CellType) (ch);
                else
                    *(machine->currentCell + cellCode->arg.half.offset) = (CellType) (ch);
                break;
            case VMOPCODE_LOOPBEGIN:
                if (*machine->currentCell != 0) break;
            case VMOPCODE_LOOPEND:
                Machine_JmpCode(machine, cellCode->arg.full);
                break;
            case VMOPCODE_SET:
                *(machine->currentCell + cellCode->arg.half.offset) = (CellType) (cellCode->arg.half.arg);
                break;
            case VMOPCODE_COPY:
                *(machine->currentCell + cellCode->arg.half.offset) += *(machine->currentCell + cellCode->arg.half.arg);
                break;
            case VMOPCODE_MOV:
                *(machine->currentCell + cellCode->arg.half.offset) = *(machine->currentCell + cellCode->arg.half.arg);
                break;
            case VMOPCODE_MUL:
                *(machine->currentCell + cellCode->arg.half.offset) += (*machine->currentCell) * cellCode->arg.half.arg;
                break;
            case VMOPCODE_SCAN:
                ptr = (&machine->cells[0] + machine->currentCellN);
                if (cellCode->arg.full < 0)
                    v = -(ptr - (char *) memrchr(&machine->cells[0], 0, machine->currentCellN + 1));
                else
                    v = (char *) memchr(&machine->cells[0] + machine->currentCellN, 0, maxCells) - ptr;
                Machine_MoveWindow(machine, v);
                break;
            case VMOPCODE_MEMSET:
                memset(&machine->currentCell[cellCode->arg.half.offset],
                       cellCode->arg.half.arg,
                       cellCode->arg.half.arg2);
                break;
        }
    }
    Machine_Free(machine);
    return 0;
}
