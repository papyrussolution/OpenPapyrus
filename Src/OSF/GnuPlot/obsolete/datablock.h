/*
 * $Id: datablock.h,v 1.3 2014/04/05 06:17:08 markisch Exp $
 */
void datablock_command();
char **get_datablock(char *name);
char *parse_datablock_name();
void gpfree_datablock(t_value *datablock_value);
void append_to_datablock(t_value *datablock_value, const char * line);
