#include "spimcore.h"
/* Functions not yet completed: 
 - instruction_partition
 - instruction_decode
 - rw_memory
 - write_register
*/

void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero)
{
    switch(ALUControl) // set up a switch case for 8 distinct ALUControls
    {
        case 0: // case 000, addition
            *ALUresult = A+B;
            break;

        case 1: // case 001, subtraction
            *ALUresult = A-B;
            break;

        case 2: // case 010, signed less than check
            *ALUresult = (int)A < (int)B;
            break;

        case 3: // case 011, unsigned less than check
            *ALUresult = A < B;
            break;

        case 4: // case 100, A AND B
            *ALUresult = A & B;
            break;

        case 5: // case 101, A OR B
            *ALUresult = A | B;
            break;

        case 6: // case 110, shift B 16 bits
            *ALUresult = B << 16;
            break;

        case 7: // case 111, NOT A
            *ALUresult = ~A;
            break;

        default: // default case, set ALUresult to 0
            *ALUresult = 0;
    }

    *Zero = (*ALUresult == 0); // assign *Zero here
}

int instruction_fetch(unsigned PC,unsigned *Mem,unsigned *instruction)
{
    // needed parentheses to get rid of error with != being checked before PC & 3
    if ((PC & 3) != 0) return 1;

    *instruction = Mem[PC>>2];

    return 0;
}

void read_register(unsigned r1,unsigned r2,unsigned *Reg,unsigned *data1,unsigned *data2)
{
    *data1 = Reg[r1];
    *data2 = Reg[r2];
}

void sign_extend(unsigned offset,unsigned *extended_value)
{
    if (offset & 0b1000000000000000) // check 16th bit of offset
        *extended_value = offset | 0b11111111111111110000000000000000; // if it's a 1, extend with 16 1s
    else
        *extended_value = offset & 0b00000000000000001111111111111111; // or extend with zeros
}

int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero)
{
    unsigned operand1 = data1; // op 1 is always data1, as MIPS requires
    unsigned operand2 = 0; // init here
    int command = 0;

    if (ALUSrc == 0)
        operand2 = data2;
    else if (ALUSrc == 1)
        operand2 = extended_value;
    // if ALUSrc is something else (like 2 for j),
    // we just leave operand2 = 0 and still run ALU.
    // The ALU result won't be used for j anyway.
    if (ALUOp == 7) // R-type
    {
        switch (funct)
        {
            case 0x20: command = 0; break; // add
            case 0x22: command = 1; break; // sub
            case 0x2A: command = 2; break; // SLT signed
            case 0x2B: command = 3; break; // SLT unsigned
            case 0x24: command = 4; break; // AND
            case 0x25: command = 5; break; //OR
            default: return 1; // invalid R-type instruction
        }
    }

    else // I-type
    {
        switch (ALUOp)
        {
            case 0: command = 0; break; //addi, lw, sw
            case 1: command = 1; break; // beq, bne
            case 2: command = 2; break; // slti
            case 3: command = 3; break; // sltiu
            case 6: command = 6; break; // lui
            default: return 1; // invalid I-type instruction
        }
    }

    ALU(operand1, operand2, command, ALUresult, Zero); // call ALU with given command

    return 0; // no halt here
}

void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC)
{
    *PC = *PC + 4; // shift PC

    if (Jump) // code for jump
    {
        jsec = jsec << 2; // shift jsec to the left by 2
        jsec = jsec & 0b00001111111111111111111111111111; // mask  lower bits of jsec
        *PC = *PC & 0b11110000000000000000000000000000; // mask first 4 bits of PC
        *PC = *PC | jsec; // combine them for the new PC
        return;
    }

    if (Branch && Zero) // code for branch
    {
        extended_value = extended_value << 2;
        *PC = *PC + extended_value;
    }
}
// Break the 32-bit instruction into its different pieces
void instruction_partition(unsigned instruction,
                           unsigned *op,
                           unsigned *r1,
                           unsigned *r2,
                           unsigned *r3,
                           unsigned *funct,
                           unsigned *offset,
                           unsigned *jsec)
{
    *op     = (instruction >> 26) & 0x3F;     // top 6 bits
    *r1     = (instruction >> 21) & 0x1F;     // rs
    *r2     = (instruction >> 16) & 0x1F;     // rt
    *r3     = (instruction >> 11) & 0x1F;     // rd
    *funct  = instruction & 0x3F;             // last 6 bits
    *offset = instruction & 0xFFFF;           // low 16 bits
    *jsec   = instruction & 0x03FFFFFF;       // jump section (26 bits)
}

// Set the control signals depending on the opcode
int instruction_decode(unsigned op, struct_controls *controls)
{
    // R-type
    if (op == 0x0) {
        controls->RegDst   = 1;
        controls->Jump     = 0;
        controls->Branch   = 0;
        controls->MemRead  = 0;
        controls->MemtoReg = 0;
        controls->ALUOp    = 7;   // use funct field
        controls->MemWrite = 0;
        controls->ALUSrc   = 0;
        controls->RegWrite = 1;
        return 0;
    }

    // j
    if (op == 0x2) {
        controls->RegDst   = 2;
        controls->Jump     = 1;
        controls->Branch   = 0;
        controls->MemRead  = 0;
        controls->MemtoReg = 2;
        controls->ALUOp    = 0;
        controls->MemWrite = 0;
        controls->ALUSrc   = 2;
        controls->RegWrite = 0;
        return 0;
    }

    // beq
    if (op == 0x4) {
        controls->RegDst   = 2;
        controls->Jump     = 0;
        controls->Branch   = 1;
        controls->MemRead  = 0;
        controls->MemtoReg = 2;
        controls->ALUOp    = 1;  // subtract
        controls->MemWrite = 0;
        controls->ALUSrc   = 0;
        controls->RegWrite = 0;
        return 0;
    }

    // addi
    if (op == 0x8) {
        controls->RegDst   = 0;
        controls->Jump     = 0;
        controls->Branch   = 0;
        controls->MemRead  = 0;
        controls->MemtoReg = 0;
        controls->ALUOp    = 0;  // add
        controls->MemWrite = 0;
        controls->ALUSrc   = 1;
        controls->RegWrite = 1;
        return 0;
    }

    // slti
    if (op == 0xA) {
        controls->RegDst   = 0;
        controls->Jump     = 0;
        controls->Branch   = 0;
        controls->MemRead  = 0;
        controls->MemtoReg = 0;
        controls->ALUOp    = 2;  // signed slt
        controls->MemWrite = 0;
        controls->ALUSrc   = 1;
        controls->RegWrite = 1;
        return 0;
    }

    // sltiu
    if (op == 0xB) {
        controls->RegDst   = 0;
        controls->Jump     = 0;
        controls->Branch   = 0;
        controls->MemRead  = 0;
        controls->MemtoReg = 0;
        controls->ALUOp    = 3;  // unsigned slt
        controls->MemWrite = 0;
        controls->ALUSrc   = 1;
        controls->RegWrite = 1;
        return 0;
    }

    // lui
    if (op == 0xF) {
        controls->RegDst   = 0;
        controls->Jump     = 0;
        controls->Branch   = 0;
        controls->MemRead  = 0;
        controls->MemtoReg = 0;
        controls->ALUOp    = 6;  // shift left 16
        controls->MemWrite = 0;
        controls->ALUSrc   = 1;
        controls->RegWrite = 1;
        return 0;
    }

    // lw
    if (op == 0x23) {
        controls->RegDst   = 0;   // rt
        controls->Jump     = 0;
        controls->Branch   = 0;
        controls->MemRead  = 1;
        controls->MemtoReg = 1;   // load from memory
        controls->ALUOp    = 0;
        controls->MemWrite = 0;
        controls->ALUSrc   = 1;
        controls->RegWrite = 1;
        return 0;
    }

    // sw
    if (op == 0x2B) {
        controls->RegDst   = 2;
        controls->Jump     = 0;
        controls->Branch   = 0;
        controls->MemRead  = 0;
        controls->MemtoReg = 2;
        controls->ALUOp    = 0;
        controls->MemWrite = 1;
        controls->ALUSrc   = 1;
        controls->RegWrite = 0;
        return 0;
    }

    // anything else → invalid instruction
    return 1;
}

// Handle loads and stores
int rw_memory(unsigned ALUresult,
              unsigned data2,
              char MemWrite,
              char MemRead,
              unsigned *memdata,
              unsigned *Mem)
{
    // load
    if (MemRead == 1) {
        // check alignment and range (64 KB memory)
        if ((ALUresult & 3) != 0 || ALUresult >= 65536)
            return 1;

        *memdata = Mem[ALUresult >> 2];
    }

    // store
    if (MemWrite == 1) {
        if ((ALUresult & 3) != 0 || ALUresult >= 65536)
            return 1;

        Mem[ALUresult >> 2] = data2;
    }

    return 0;
}

// Write the final value back into a register
void write_register(unsigned r2,
                    unsigned r3,
                    unsigned memdata,
                    unsigned ALUresult,
                    char RegWrite,
                    char RegDst,
                    char MemtoReg,
                    unsigned *Reg)
{
    // skip if we're not writing a register
    if (RegWrite != 1)
        return;

    unsigned dest;
    unsigned value;

    // pick register to write:
    dest = (RegDst == 1) ? r3 : r2;

    // choose value to write:
    value = (MemtoReg == 1) ? memdata : ALUresult;

    // don't write to $0
    if (dest != 0)
        Reg[dest] = value;
}
