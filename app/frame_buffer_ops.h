#ifndef FRAME_BUFFER_OPS_H
#define FRAME_BUFFER_OPS_H

#include "session.h"
#include "frame_buffer.h"

void fb_init(session *sess, frame_buffer *fb);
void fb_free(frame_buffer *fb);
void fb_update(session *sess, frame_buffer *fb);
void fb_update_clut(session *sess, frame_buffer *fb);

void fb_reset_dirty_cursor(frame_buffer *fb);
void fb_advance_dirty_cursor(frame_buffer *fb);
int fb_size_at_dirty_cursor(frame_buffer *fb);
int fb_more_dirty(frame_buffer *fb);
char* fb_dirty_cursor_ptr(frame_buffer *fb);

#endif