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
    if (PC & 3 != 0) return 1;

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
    unsigned operand2; // init here
    int command = 0;

    if (ALUSrc == 0)
        operand2 = data2;
    else if (ALUSrc == 1)
        operand2 = extended_value;
    else
        return 1; // not sure if this is truly necessary but halt if somehow a ALUSrc is set outside [0,1]

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
