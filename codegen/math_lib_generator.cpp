#include <stdio.h>

#define ARRAY_COUNT(arr) sizeof(arr) / sizeof((arr)[0])
#define TAB "    "

// NOTE(dima): Vectors

void generateVectorDefinition(FILE *file, char *dim, int size)
{
    fprintf(file, "struct V%d\n", size);
    fprintf(file, "{\n");
    fprintf(file, "%sr32 ", TAB);
    for (int i = 0; i < size; ++i)
    {
        if (i == size - 1)
        {
            fprintf(file, "%c;\n", dim[i]);
        }
        else
        {
            fprintf(file, "%c, ", dim[i]);
        }
    }
    fprintf(file, "%sr32 operator[](s32 index) { return ((&x)[index]); }\n", TAB);
    fprintf(file, "\n");
    fprintf(file, "%sinline V%d &operator*=(r32 a);\n", TAB, size);
    fprintf(file, "%sinline V%d &operator+=(V%d a);\n", TAB, size, size);
    fprintf(file, "};\n");
    fprintf(file, "\n");
}

void generateVectorFactoryFunction(FILE *file, char *dim, int size)
{
    fprintf(file, "inline V%d v%d(", size, size);
    for (int i = 0; i < size; ++i)
    {
        if (i == size - 1)
        {
            fprintf(file, "r32 %c)\n", dim[i]);
        }
        else
        {
            fprintf(file, "r32 %c, ", dim[i]);
        }
    }
    fprintf(file, "{\n");
    fprintf(file, "%sV%d result;\n", TAB, size);
    fprintf(file, "\n");
    for (int i = 0; i < size; ++i)
    {
        fprintf(file, "%sresult.%c = %c;\n", TAB, dim[i], dim[i]);
    }
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void generateVectorFactoryFunction2(FILE *file, char *dim, int size)
{
    fprintf(file, "inline V%d v%d(V%d a, r32 val)\n", size, size, size - 1);
    fprintf(file, "{\n");
    fprintf(file, "%sV%d result;\n", TAB, size);
    fprintf(file, "\n");
    for (int i = 0; i < size; ++i)
    {
        if (i == size - 1)
        {
            fprintf(file, "%sresult.%c = val;\n", TAB, dim[i]);
        }
        else
        {
            fprintf(file, "%sresult.%c = a.%c;\n", TAB, dim[i], dim[i]);
        }
    }
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void generateVectorFactoryFunction3(FILE *file, char *dim, int size)
{
    fprintf(file, "inline V%d v%d(V%d a)\n", size - 1, size - 1, size);
    fprintf(file, "{\n");
    fprintf(file, "%sV%d result;\n", TAB, size - 1);
    fprintf(file, "\n");
    for (int i = 0; i < size - 1; ++i)
    {
        fprintf(file, "%sresult.%c = a.%c;\n", TAB, dim[i], dim[i]);
    }
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void generateMultiplyScalarOverride(FILE *file, char *dim, int size)
{
    // NOTE(dima): scalar on the left
    fprintf(file, "inline V%d operator*(r32 a, V%d b)\n", size, size);
    fprintf(file, "{\n");
    fprintf(file, "%sV%d result;\n", TAB, size);
    fprintf(file, "\n");
    for (int i = 0; i < size; ++i)
    {
        fprintf(file, "%sresult.%c = a * b.%c;\n", TAB, dim[i], dim[i]);
    }
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");

    // NOTE(dima): scalar on the right
    fprintf(file, "inline V%d operator*(V%d b, r32 a)\n", size, size);
    fprintf(file, "{\n");
    fprintf(file, "%sV%d result;\n", TAB, size);
    fprintf(file, "\n");
    for (int i = 0; i < size; ++i)
    {
        fprintf(file, "%sresult.%c = a * b.%c;\n", TAB, dim[i], dim[i]);
    }
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void generateMultiplyEqualScalarOverride(FILE *file, int size)
{
    fprintf(file, "inline V%d &V%d::\noperator*=(r32 a)\n", size, size);
    fprintf(file, "{\n");
    fprintf(file, "%s*this = a * *this;\n", TAB);
    fprintf(file, "\n");
    fprintf(file, "%sreturn *this;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}
 
void generateSubtractOverride(FILE *file, char *dim, int size)
{
    fprintf(file, "inline V%d operator-(V%d a, V%d b)\n", size, size, size);
    fprintf(file, "{\n");
    fprintf(file, "%sV%d result;\n", TAB, size);
    fprintf(file, "\n");
    for (int i = 0; i < size; ++i)
    {
        fprintf(file, "%sresult.%c = a.%c - b.%c;\n", TAB, dim[i], dim[i], dim[i]);
    }
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void generateAddOverride(FILE *file, char *dim, int size)
{
    fprintf(file, "inline V%d operator+(V%d a, V%d b)\n", size, size, size);
    fprintf(file, "{\n");
    fprintf(file, "%sV%d result;\n", TAB, size);
    fprintf(file, "\n");
    for (int i = 0; i < size; ++i)
    {
        fprintf(file, "%sresult.%c = a.%c + b.%c;\n", TAB, dim[i], dim[i], dim[i]);
    }
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void generateDotProduct(FILE *file, char *dim, int size)
{
    fprintf(file, "inline r32 dot(V%d a, V%d b)\n", size, size);
    fprintf(file, "{\n");
    fprintf(file, "%sr32 result;\n", TAB);
    fprintf(file, "\n");

    fprintf(file, "%sresult = ", TAB);
    for (int i = 0; i < size; ++i)
    {
        if (i == size - 1)
        {
            fprintf(file, "a.%c * b.%c;\n", dim[i], dim[i]);
        }
        else
        {
            fprintf(file, "a.%c * b.%c + ", dim[i], dim[i]);
        }
    }
    fprintf(file, "\n");

    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void generateCrossProductV3(FILE *file)
{
    fprintf(file, "inline V3 cross(V3 a, V3 b)\n");
    fprintf(file, "{\n");
    fprintf(file, "%sV3 result;\n", TAB);
    fprintf(file, "\n");
    fprintf(file, "%sresult.x = a.y * b.z - a.z * b.y;\n", TAB);
    fprintf(file, "%sresult.y = a.z * b.x - a.x * b.z;\n", TAB);
    fprintf(file, "%sresult.z = a.x * b.y - a.y * b.x;\n", TAB);
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void generateSquareLength(FILE *file, int size)
{
    fprintf(file, "inline r32 squareLength(V%d a)\n", size);
    fprintf(file, "{\n");
    fprintf(file, "%sr32 result = dot(a, a);\n", TAB);
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void generateLength(FILE *file, int size)
{
    fprintf(file, "inline r32 length(V%d a)\n", size);
    fprintf(file, "{\n");
    fprintf(file, "%sr32 result = dot(a, a);\n", TAB);
    fprintf(file, "#if COMPILER_LLVM\n");
    fprintf(file, "%sresult = __builtin_sqrt(result);\n", TAB);
    fprintf(file, "#else\n");
    fprintf(file, "#include <math.h>\n");
    fprintf(file, "%sresult = sqrt(result);\n", TAB);
    fprintf(file, "#endif");
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

// NOTE(dima): Matrices

void generateMatrixDefinition(FILE *file, int size)
{
    fprintf(file, "struct M%d%d\n", size, size);
    fprintf(file, "{\n");
    fprintf(file, "%sr32 cols[%d];\n", TAB, size * size);
    fprintf(file, "%sV%d *operator[](s32 index)\n", TAB, size);
    fprintf(file, "%s{\n", TAB);
    fprintf(file, "%s%sASSERT(index >= 0 && index < %d);\n", TAB, TAB, size);
    fprintf(file, "%s%sr32 *col = cols + index * %d;\n", TAB, TAB, size);
    fprintf(file, "\n");
    fprintf(file, "%s%sreturn (V%d *) col;\n", TAB, TAB, size);
    fprintf(file, "%s}\n", TAB);

    if (size == 4)
    {
        fprintf(file, "\n");
        fprintf(file, "%sV3 xyz(s32 index)\n", TAB);
        fprintf(file, "%s{\n", TAB);
        fprintf(file, "%s%sASSERT(index >= 0 && index < 4);\n", TAB, TAB);
        fprintf(file, "%s%sV4 *v4 = (V4 *) (*this)[index];\n", TAB, TAB);
        fprintf(file, "%s%sV3 result = v3(*v4);\n", TAB, TAB);
        fprintf(file, "\n");
        fprintf(file, "%s%sreturn result;\n", TAB, TAB);
        fprintf(file, "%s}\n", TAB);
    }

    fprintf(file, "};\n");
    fprintf(file, "\n");
}

void generateMatrixTranspose(FILE *file, char *dim, int size)
{
    fprintf(file, "inline M%d%d transpose(M%d%d a)\n", size, size, size, size);
    fprintf(file, "{\n");
    fprintf(file, "%sM%d%d result;\n", TAB, size, size);
    fprintf(file, "%sfor (int i = 0; i < %d; i++)\n", TAB, size);
    fprintf(file, "%s{\n", TAB);
    fprintf(file, "%s%sV%d *col = a[i];\n", TAB, TAB, size);
    for (int i = 0; i < size; i++)
    {
        fprintf(file, "%s%sresult.cols[i + %d] = col->%c;\n", TAB, TAB, i * size, dim[i]);
    }
    fprintf(file, "%s}\n", TAB);
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void generateMultiplyMatrixByMatrixOverride(FILE *file, char *dim, int size)
{
    fprintf(file, "inline M%d%d operator*(M%d%d a, M%d%d b)\n", size, size, size, size, size, size);
    fprintf(file, "{\n");
    fprintf(file, "%sM%d%d result;\n", TAB, size, size);
    fprintf(file, "\n");
    fprintf(file, "%sM%d%d aT = transpose(a);\n", TAB, size, size);
    fprintf(file, "\n");
    fprintf(file, "%sfor (int i = 0; i < %d; i++)\n", TAB, size);
    fprintf(file, "%s{\n", TAB);
    for (int i = 0; i < size; i++)
    {
        fprintf(file, "%s%sr32 dot%d = dot(*aT[%d], *b[i]);\n", TAB, TAB, i, i);
    }
    fprintf(file, "\n");
    for (int i = 0; i < size; i++)
    {
        fprintf(file, "%s%sresult.cols[i * %d + %d] = dot%d;\n", TAB, TAB, size, i, i);
    }
    fprintf(file, "%s}\n", TAB);
    fprintf(file, "\n");
    fprintf(file, "%sreturn result;\n", TAB);
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

int main(int argc, char **argv)
{
    FILE *result = fopen("antiqua_math.h", "w+");

    fprintf(result, "#ifndef _ANTIQUA_MATH_H_\n");
    fprintf(result, "#define _ANTIQUA_MATH_H_\n");
    fprintf(result, "\n");
    fprintf(result, "#include \"types.h\"\n");
    fprintf(result, "\n");

    char v2[2] = {'x', 'y'};
    char v3[3] = {'x', 'y', 'z'};
    char v4[4] = {'x', 'y', 'z', 'w'};

    fprintf(result, "// NOTE(dima): Vector definitions\n\n");
    generateVectorDefinition(result, v2, ARRAY_COUNT(v2));
    generateVectorDefinition(result, v3, ARRAY_COUNT(v3));
    generateVectorDefinition(result, v4, ARRAY_COUNT(v4));

    fprintf(result, "// NOTE(dima): Vector factory functions\n\n");
    generateVectorFactoryFunction(result, v2, ARRAY_COUNT(v2));
    generateVectorFactoryFunction(result, v3, ARRAY_COUNT(v3));
    generateVectorFactoryFunction(result, v4, ARRAY_COUNT(v4));

    generateVectorFactoryFunction2(result, v3, ARRAY_COUNT(v3));
    generateVectorFactoryFunction2(result, v4, ARRAY_COUNT(v4));

    generateVectorFactoryFunction3(result, v3, ARRAY_COUNT(v3));
    generateVectorFactoryFunction3(result, v4, ARRAY_COUNT(v4));

    fprintf(result, "// NOTE(dima): Operator override: multiply by scalar\n\n");
    generateMultiplyScalarOverride(result, v2, ARRAY_COUNT(v2));
    generateMultiplyScalarOverride(result, v3, ARRAY_COUNT(v3));
    generateMultiplyScalarOverride(result, v4, ARRAY_COUNT(v4));

    fprintf(result, "// NOTE(dima): Operator override: multiply equal by scalar\n\n");
    generateMultiplyEqualScalarOverride(result, ARRAY_COUNT(v2));
    generateMultiplyEqualScalarOverride(result, ARRAY_COUNT(v3));
    generateMultiplyEqualScalarOverride(result, ARRAY_COUNT(v4));

    fprintf(result, "// NOTE(dima): Operator override: subtract a vector\n\n");
    generateSubtractOverride(result, v2, ARRAY_COUNT(v2));
    generateSubtractOverride(result, v3, ARRAY_COUNT(v3));
    generateSubtractOverride(result, v4, ARRAY_COUNT(v4));

    fprintf(result, "// NOTE(dima): Operator override: add a vector\n\n");
    generateAddOverride(result, v2, ARRAY_COUNT(v2));
    generateAddOverride(result, v3, ARRAY_COUNT(v3));
    generateAddOverride(result, v4, ARRAY_COUNT(v4));

    fprintf(result, "// NOTE(dima): Dot product\n\n");
    generateDotProduct(result, v2, ARRAY_COUNT(v2));
    generateDotProduct(result, v3, ARRAY_COUNT(v3));
    generateDotProduct(result, v4, ARRAY_COUNT(v4));

    fprintf(result, "// NOTE(dima): Cross product\n\n");
    generateCrossProductV3(result);

    fprintf(result, "// NOTE(dima): Vector square length\n\n");
    generateSquareLength(result, ARRAY_COUNT(v2));
    generateSquareLength(result, ARRAY_COUNT(v3));
    generateSquareLength(result, ARRAY_COUNT(v4));

    fprintf(result, "// NOTE(dima): Vector length\n\n");
    generateLength(result, ARRAY_COUNT(v2));
    generateLength(result, ARRAY_COUNT(v3));
    generateLength(result, ARRAY_COUNT(v4));

    fprintf(result, "// NOTE(dima): Matrix definitions\n\n");
    fprintf(result, "// NOTE(dima): all matrices are in column-major order\n\n");

    generateMatrixDefinition(result, 3);
    generateMatrixDefinition(result, 4);

    fprintf(result, "// NOTE(dima): Matrix transpose\n\n");
    generateMatrixTranspose(result, v3, ARRAY_COUNT(v3));
    generateMatrixTranspose(result, v4, ARRAY_COUNT(v4));

    fprintf(result, "// NOTE(dima): Operator override: multiply matrix by matrix\n\n");
    generateMultiplyMatrixByMatrixOverride(result, v3, ARRAY_COUNT(v3));
    generateMultiplyMatrixByMatrixOverride(result, v4, ARRAY_COUNT(v4));

//    TODO for later: generate matrix inverse
//    TODO for later: generate matrix vector multiplication

    fprintf(result, "#endif\n");

    return 0;
}
