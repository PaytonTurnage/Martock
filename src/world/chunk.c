#include <martock.h>

/**
 *  Load a chunk from file if it exists.
 *
 *  @pos: the position of the chunk in the world
 *
 *  @return: the loaded chunk; NULL on failure
 */
chunk *chunk_load (u16 pos)
{
        chunk *ch = NULL;
        FILE *file = NULL;

        /* Return if the memory allocation fails. */
        if (!(ch = calloc(1, sizeof(chunk))))
                return NULL;

        /* Return if the file can't be found. */
        if (!(file = vfopen("rb", "world/chunks/%d.ch", pos)))
                return NULL;

        fread(ch, sizeof(chunk), 1, file);
        return ch;
}

/**
 *  Construct a new chunk according the provided rules using cellular automata.
 *
 *  @rules: cellular automata rules
 *  @neighbor: the chunk this is appending to
 *  @side: which side it's appending to the neighbor
 *
 *  @return: the newly generated chunk; NULL on failure
 */
chunk *chunk_generate (u8 rules, const chunk *neighbor, u8 side)
{
        chunk *ch = NULL;

        /* Return if the memory allocation fails. */
        if (!(ch = calloc(1, sizeof(chunk))))
                return NULL;

        /* Set the defaults for each vertical level. */
        for (int i = 0; i < CHUNK_WIDTH; i++)
                for (int j = 0; j < CHUNK_SOIL; j++)
                        ch->grid[i][j].id = BLOCK_SKY;

        for (int i = 0; i < CHUNK_WIDTH; i++)
                for (int j = CHUNK_SOIL; j < CHUNK_MANTLE; j++)
                        ch->grid[i][j].id = BLOCK_SOIL;

        for (int i = 0; i < CHUNK_WIDTH; i++)
                for (int j = CHUNK_MANTLE; j < CHUNK_HEIGHT; j++)
                        ch->grid[i][j].id = BLOCK_STONE;

        /* If a flat chunk was requested, it's done. */
        if (rules & CHUNK_FLAT)
                return ch;

        /* Gradient stone. */
        double total = CHUNK_MANTLE - CHUNK_SOIL;
        for (int i = 0; i < CHUNK_WIDTH; i++)
                for (int j = CHUNK_SOIL + 1; j < CHUNK_MANTLE; j++) {
                        int prop = ((j - CHUNK_SOIL) / total) * 100;
                        if ((rand() % 100) < prop)
                                ch->grid[i][j].id = BLOCK_STONE;
                }

        /* Initial seeding. */
        double ytotal = CHUNK_CORE - CHUNK_MANTLE;
        double xtotal = CHUNK_WIDTH - CHUNK_BORDER;
        for (int i = CHUNK_BORDER; i < CHUNK_WIDTH - CHUNK_BORDER; i++)
                for (int j = CHUNK_MANTLE; j < CHUNK_CORE; j++) {
                        int yprop = ((j - CHUNK_MANTLE) / ytotal) * 100;
                        int xprop = ((i - CHUNK_BORDER) / xtotal) * 100;
                        yprop = (yprop > 50) ? 100 - yprop : yprop;
                        xprop = (xprop > 50) ? 100 - xprop : xprop;
                        if ((rand() % 100) < ((xprop + yprop) / 2))
                                ch->grid[i][j].id = BLOCK_SKY;
                }

        /* Cave automata. */
        chunk cmp;
        memcpy(&cmp, ch, sizeof(chunk));
        for (int c = 0; c < CHUNK_CAVE_DEPTH; c++)
                for (int i = CHUNK_BORDER; i < CHUNK_WIDTH - CHUNK_BORDER; i++)
                        for (int j = CHUNK_MANTLE; j < CHUNK_CORE; j++) {
                                int t = 0;

                                t = (cmp.grid[i - 1][j - 1].id) ? (t + 1) : t;
                                t = (cmp.grid[i - 1][j    ].id) ? (t + 1) : t;
                                t = (cmp.grid[i - 1][j + 1].id) ? (t + 1) : t;

                                t = (cmp.grid[i    ][j - 1].id) ? (t + 1) : t;
                                t = (cmp.grid[i    ][j + 1].id) ? (t + 1) : t;

                                t = (cmp.grid[i + 1][j - 1].id) ? (t + 1) : t;
                                t = (cmp.grid[i + 1][j    ].id) ? (t + 1) : t;
                                t = (cmp.grid[i + 1][j + 1].id) ? (t + 1) : t;

                                if (ch->grid[i][j].id) {
                                        if ((t < 3) || (t > 8))
                                                ch->grid[i][j].id = BLOCK_SKY;
                                } else
                                        if ((t > 5) && (t < 9))
                                                ch->grid[i][j].id = BLOCK_STONE;
                        }

        return ch;
}

/**
 *  Render a chunk to an image file to preview it. This function is inexplicably
 *  slow. Leave it running overnight, or depend on the text render.
 *
 *  @ch: the chunk to preview
 */
void chunk_save_img (chunk *ch)
{
        if (!ch)
                return;

        int scale = BLOCK_SIZE / BLOCK_SCALE;
        int width = scale * CHUNK_WIDTH;
        int height = scale * CHUNK_HEIGHT;

        ALLEGRO_BITMAP *canvas = al_create_bitmap(width, height);

        if (!canvas)
                return;

        al_set_target_bitmap(canvas);

        for (int i = 0; i < CHUNK_WIDTH; i++)
                for (int j = 0; j < CHUNK_HEIGHT; j++) {
                        al_draw_scaled_bitmap(block_sprite(ch->grid[i][j].id),
                                              0, 0, BLOCK_SIZE, BLOCK_SIZE,
                                              i * scale, j * scale,
                                              scale, scale, 0);
                }

        char temp[100] = {0};
        sprintf(temp, "world/chunks/%d.png", ch->position);
        al_save_bitmap(temp, canvas);
        al_destroy_bitmap(canvas);
}

/**
 *  Create a lingual save representing the chunk visually with text.
 *
 *  @ch: pointer to the chunk to save
 */
void chunk_save_text (const chunk *ch)
{
        if (!ch)
                return;

        FILE *file = vfopen("w", "world/chunks/%d.txt", ch->position);

        if (file)
                for (int j = 0; j < CHUNK_HEIGHT; j++) {
                        for (int i = 0; i < CHUNK_WIDTH; i++)
                                switch (ch->grid[i][j].id) {
                                        case BLOCK_SKY:   fprintf(file, "  ");
                                                          break;
                                        case BLOCK_GRASS: fprintf(file, "--");
                                                          break;
                                        case BLOCK_SOIL:  fprintf(file, "..");
                                                          break;
                                        case BLOCK_STONE: fprintf(file, "##");
                                                          break;
                                        default:          fprintf(file, "%2d",
                                                          ch->grid[i][j].id);
                                }
                        fprintf(file, "\n");
                }
}

/**
 *  Save the data from the chunk if a save file structure is present, and then
 *  free the memory used by the chunk.
 *
 *  @ch: pointer to the chunk memory
 */
void chunk_close (chunk *ch)
{
        if (!ch)
                return;

        FILE *file = vfopen("wb", "world/chunks/%d.ch", ch->position);

        if (file)
                fwrite(ch, sizeof(chunk), 1, file);

        free(ch);
}