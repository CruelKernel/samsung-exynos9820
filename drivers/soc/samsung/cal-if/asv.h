#ifndef __ASV_H__
#define __ASV_H__

extern int asv_table_init(void);
extern int asv_get_table_ver(void);
extern int asv_get_grp(unsigned int id);
extern int asv_get_ids_info(unsigned int id);
extern void id_get_rev(unsigned int *main_rev, unsigned int *sub_rev);

extern int id_get_product_line(void);
extern int id_get_asb_ver(void);
#endif
