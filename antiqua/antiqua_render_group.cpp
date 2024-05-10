#include <cstring>

#include "antiqua_render_group.h"

void pushRenderEntryClear(RenderGroup *renderGroup, V3 color)
{
    ASSERT(renderGroup->pushBufferElementCount == 0);

    RenderGroupEntryHeader *entryHeader = (RenderGroupEntryHeader *)(renderGroup->pushBufferBase + renderGroup->pushBufferSize);
    entryHeader->type = RenderGroupEntryType_RenderEntryClear;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryClear *entry = (RenderEntryClear *)(renderGroup->pushBufferBase + renderGroup->pushBufferSize);
    entry->color = v4(color, 1.0f);
    renderGroup->pushBufferSize += sizeof(RenderEntryClear);
    renderGroup->pushBufferElementCount++;

    /* NOTE(dima): have to set next pointer to null deliberately,
                   because we are reusing memory from previous frame. If that memory had something
                   before, it will break our iteration over list of render entries */
    entryHeader->next = 0;
    renderGroup->prevHeader = entryHeader;
}

void pushRenderEntryLine(RenderGroup *renderGroup, V3 color, V3 start, V3 end)
{
    ASSERT(renderGroup->pushBufferElementCount > 0);

    RenderGroupEntryHeader *entryHeader = (RenderGroupEntryHeader *)(renderGroup->pushBufferBase + renderGroup->pushBufferSize);
    entryHeader->type = RenderGroupEntryType_RenderEntryLine;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryLine *entry = (RenderEntryLine *)(renderGroup->pushBufferBase + renderGroup->pushBufferSize);
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

void pushRenderEntryMesh(RenderGroup *renderGroup, r32 *vertices, u32 totalCountOfElementsInArray)
{
    ASSERT(renderGroup->pushBufferElementCount > 0);

    RenderGroupEntryHeader *entryHeader = (RenderGroupEntryHeader *)(renderGroup->pushBufferBase + renderGroup->pushBufferSize);
    entryHeader->type = RenderGroupEntryType_RenderEntryMesh;
    renderGroup->pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryMesh *entry = (RenderEntryMesh *)(renderGroup->pushBufferBase + renderGroup->pushBufferSize);
    entry->data = vertices;
    entry->size = sizeof(r32) * totalCountOfElementsInArray;
    renderGroup->pushBufferSize += sizeof(RenderEntryMesh);
    renderGroup->pushBufferElementCount++;

    renderGroup->prevHeader->next = entryHeader;
    renderGroup->prevHeader = entryHeader;
    entryHeader->next = 0;
}
