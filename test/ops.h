#ifndef _OPS_H_
#define _OPS_H_

char *get_path(const char *base, const char *name);
int write_ops(const char *pathname, unsigned short data);
int read_block_ops(const char *pathname, char **buf);
int read_ops(const char *pathname, unsigned short *data);
int write_data(const char *base, const char *name, unsigned short data);
int read_data(const char *base, const char *name, unsigned short *data);
int read_data_block(const char *base, const char *name, char **buf);

#endif /* _OPS_H */
