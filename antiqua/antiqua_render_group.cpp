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

void pushRenderEntryTile(MemoryArena *arena,
                         RenderGroup *renderGroup,
                         u32 tileCountPerSide,
                         r32 tileSideLength,
                         V3 originTileCenterPositionWorld,
                         V3 color)
{
    ASSERT(renderGroup->pushBufferElementCount > 0);

    RenderGroupEntryHeader *entryHeader = PUSH_STRUCT(arena, RenderGroupEntryHeader);
    entryHeader->type = RenderGroupEntryType_RenderEntryTile;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryTile *entry = PUSH_STRUCT(arena, RenderEntryTile);
    entry->tileCountPerSide = tileCountPerSide;
    entry->tileSideLength = tileSideLength;
    entry->color = color;
    entry->originTileCenterPositionWorld = originTileCenterPositionWorld;
    renderGroup->pushBufferSize += sizeof(RenderEntryTile);
    renderGroup->pushBufferElementCount++;

    renderGroup->prevHeader->next = entryHeader;
    renderGroup->prevHeader = entryHeader;
    entryHeader->next = 0;
}

void pushRenderEntryRect(MemoryArena *arena,
                         RenderGroup *renderGroup,
                         V3 rectCenterPositionWorld,
                         r32 scaledSideLengthW,
                         r32 scaledSideLengthH)
{
    ASSERT(renderGroup->pushBufferElementCount > 0);

    RenderGroupEntryHeader *entryHeader = PUSH_STRUCT(arena, RenderGroupEntryHeader);
    entryHeader->type = RenderGroupEntryType_RenderEntryRect;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryRect *entry = PUSH_STRUCT(arena, RenderEntryRect);
    entry->sideLengthW = scaledSideLengthW;
    entry->sideLengthH = scaledSideLengthH;
    entry->rectCenterPositionWorld = rectCenterPositionWorld;
    entry->color = v3(0.0f, 1.0f, 1.0f);

    renderGroup->pushBufferSize += sizeof(RenderEntryRect);
    renderGroup->pushBufferElementCount++;

    renderGroup->prevHeader->next = entryHeader;
    renderGroup->prevHeader = entryHeader;
    entryHeader->next = 0;
}

void pushRenderEntryTextureDebug(MemoryArena *arena,
                                 RenderGroup *renderGroup,
                                 AssetHeader *textureHeader)
{
    ASSERT(renderGroup->pushBufferElementCount > 0);

    RenderGroupEntryHeader *entryHeader = PUSH_STRUCT(arena, RenderGroupEntryHeader);
    entryHeader->type = RenderGroupEntryType_RenderEntryTextureDebug;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryTextureDebug *entry = PUSH_STRUCT(arena, RenderEntryTextureDebug);
    entry->needsGpuReupload = true;
    entry->textureHeader = textureHeader;

    renderGroup->pushBufferSize += sizeof(RenderEntryTextureDebug);
    renderGroup->pushBufferElementCount++;

    renderGroup->prevHeader->next = entryHeader;
    renderGroup->prevHeader = entryHeader;
    entryHeader->next = 0;
}
