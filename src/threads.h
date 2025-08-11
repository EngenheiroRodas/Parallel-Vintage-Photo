#ifndef THREADS_H
#define THREADS_H

extern struct timespec total_pic_time;

extern gdImagePtr in_texture_img;

void *process_image(void *arg);

void *handle_key_press(void *arg);

#endif