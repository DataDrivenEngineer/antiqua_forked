#include <cstring>

#include "antiqua_render_group.h"

void pushRenderEntryClear(MemoryArena *arena,
                          RenderGroup *renderGroup,
                          V3 color)
{
    ASSERT(renderGroup->pushBufferElementCount == 0);

    RenderGroupEntryHeader *entryHeader =
        (RenderGroupEntryHeader *)PUSH_STRUCT(arena, RenderGroupEntryHeader);
    entryHeader->type = RenderGroupEntryType_RenderEntryClear;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryClear *entry =
        (RenderEntryClear *)PUSH_STRUCT(arena, RenderEntryClear);
    entry->color = v4(color, 1.0f);
    renderGroup->pushBufferSize += sizeof(RenderEntryClear);
    renderGroup->pushBufferElementCount++;

    /* NOTE(dima): have to set next pointer to null deliberately,
                   because we are reusing memory from previous frame. If that memory had something
                   before, it will break our iteration over list of render entries */
    entryHeader->next = 0;
    renderGroup->prevHeader = entryHeader;
}

void pushRenderEntryPoint(MemoryArena *arena,
                          RenderGroup *renderGroup,
                          V3 position,
                          V3 color)
{
    ASSERT(renderGroup->pushBufferElementCount > 0);

    RenderGroupEntryHeader *entryHeader =
        (RenderGroupEntryHeader *)PUSH_STRUCT(arena, RenderGroupEntryHeader);
    entryHeader->type = RenderGroupEntryType_RenderEntryPoint;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryPoint *entry =
        (RenderEntryPoint *)PUSH_STRUCT(arena, RenderEntryPoint);
    entry->position = position;
    entry->color = color;
    renderGroup->pushBufferSize += sizeof(RenderEntryPoint);
    renderGroup->pushBufferElementCount++;

    renderGroup->prevHeader->next = entryHeader;
    renderGroup->prevHeader = entryHeader;
    entryHeader->next = 0;
}

void pushRenderEntryLine(MemoryArena *arena,
                         RenderGroup *renderGroup,
                         V3 color,
                         V3 start,
                         V3 end)
{
    ASSERT(renderGroup->pushBufferElementCount > 0);

    RenderGroupEntryHeader *entryHeader =
        (RenderGroupEntryHeader *)PUSH_STRUCT(arena, RenderGroupEntryHeader);
    entryHeader->type = RenderGroupEntryType_RenderEntryLine;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryLine *entry = (RenderEntryLine *)PUSH_STRUCT(arena, RenderEntryLine);
    // NOTE(dima): 3 floats (start) + 3 floats (start color) + 3 floats (end) + 3 floats (end color)
    entry->color = color;
    entry->start = start;
    entry->end = end;
    renderGroup->pushBufferSize += sizeof(RenderEntryLine);
    renderGroup->pushBufferElementCount++;

    renderGroup->prevHeader->next = entryHeader;
    renderGroup->prevHeader = entryHeader;
    entryHeader->next = 0;
}

void pushRenderEntryMesh(MemoryArena *arena,
                         RenderGroup *renderGroup,
                         r32 *vertices,
                         u32 totalCountOfElementsInArray)
{
    ASSERT(renderGroup->pushBufferElementCount > 0);

    RenderGroupEntryHeader *entryHeader =
        (RenderGroupEntryHeader *)PUSH_STRUCT(arena, RenderGroupEntryHeader);
    entryHeader->type = RenderGroupEntryType_RenderEntryMesh;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryMesh *entry =
        (RenderEntryMesh *)(PUSH_STRUCT(arena, RenderEntryMesh));
    entry->size = sizeof(r32) * totalCountOfElementsInArray;
    entry->data = vertices;
    renderGroup->pushBufferSize += sizeof(RenderEntryMesh);
    renderGroup->pushBufferElementCount++;

    renderGroup->prevHeader->next = entryHeader;
    renderGroup->prevHeader = entryHeader;
    entryHeader->next = 0;
}

void pushRenderEntryTile(MemoryArena *arena,
                         RenderGroup *renderGroup,
                         u32 tileCountPerSide,
                         V3 originTileCenterPositionWorld,
                         V3 color)
{
    ASSERT(renderGroup->pushBufferElementCount > 0);

    RenderGroupEntryHeader *entryHeader = PUSH_STRUCT(arena, RenderGroupEntryHeader);
    entryHeader->type = RenderGroupEntryType_RenderEntryTile;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryTile *entry = PUSH_STRUCT(arena, RenderEntryTile);
    entry->tileCountPerSide = tileCountPerSide;
    entry->tileSideLength = 1.0f;
    entry->color = color;
    entry->originTileCenterPositionWorld = originTileCenterPositionWorld;
    renderGroup->pushBufferSize += sizeof(RenderEntryTile);
    renderGroup->pushBufferElementCount++;

    renderGroup->prevHeader->next = entryHeader;
    renderGroup->prevHeader = entryHeader;
    entryHeader->next = 0;
}
