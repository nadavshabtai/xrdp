/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 *
 * @file sesrun.c
 * @brief An utility to start a session
 * @author Jay Sorg, Simone Fedele
 *
 */

#include "sesman.h"
#include "tcp.h"

int g_sck;
int g_pid;
struct config_sesman g_cfg; /* config.h */

/******************************************************************************/
int DEFAULT_CC
main(int argc, char **argv)
{
    int sck;
    int code;
    int i;
    int size;
    int version;
    int width;
    int height;
    int bpp;
    int display;
    struct stream *in_s;
    struct stream *out_s;
    char *username;
    char *password;
    long data;

    if (0 != config_read(&g_cfg))
    {
        g_printf("sesrun: error reading config. quitting.\n");
        return 1;
    }

    g_pid = g_getpid();

    if (argc == 1)
    {
        g_printf("xrdp session starter v0.1\n");
        g_printf("\nusage:\n");
        g_printf("sesrun <server> <username> <password> <width> <height> <bpp>\n");
    }
    else if (argc == 7)
    {
        username = argv[2];
        password = argv[3];
        width = g_atoi(argv[4]);
        height = g_atoi(argv[5]);
        bpp = g_atoi(argv[6]);
        make_stream(in_s);
        init_stream(in_s, 8192);
        make_stream(out_s);
        init_stream(out_s, 8192);

        sck = g_tcp_socket();
        if (sck < 0)
            return 1;

        if (g_tcp_connect(sck, argv[1], g_cfg.listen_port) == 0)
        {
            s_push_layer(out_s, channel_hdr, 8);
            out_uint16_be(out_s, 0); /* code */
            i = g_strlen(username);
            out_uint16_be(out_s, i);
            out_uint8a(out_s, username, i);
            i = g_strlen(password);
            out_uint16_be(out_s, i);
            out_uint8a(out_s, password, i);
            out_uint16_be(out_s, width);
            out_uint16_be(out_s, height);
            out_uint16_be(out_s, bpp);
            s_mark_end(out_s);
            s_pop_layer(out_s, channel_hdr);
            out_uint32_be(out_s, 0); /* version */
            out_uint32_be(out_s, out_s->end - out_s->data); /* size */
            tcp_force_send(sck, out_s->data, out_s->end - out_s->data);

            if (tcp_force_recv(sck, in_s->data, 8) == 0)
            {
                in_uint32_be(in_s, version);
                in_uint32_be(in_s, size);
                init_stream(in_s, 8192);

                if (tcp_force_recv(sck, in_s->data, size - 8) == 0)
                {
                    if (version == 0)
                    {
                        in_uint16_be(in_s, code);

                        if (code == 3)
                        {
                            in_uint16_be(in_s, data);
                            in_uint16_be(in_s, display);
                            g_printf("ok %d display %d\n", data, display);
                        }
                    }
                }
            }
        }
        else
        {
            g_printf("connect error\n");
        }

        g_tcp_close(sck);
        free_stream(in_s);
        free_stream(out_s);
    }

    return 0;
}
