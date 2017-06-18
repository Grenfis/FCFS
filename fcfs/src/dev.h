#ifndef DEV_H
#define DEV_H

#include "fcfs.h"
#include <fcfs_structs.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
    Обозначает требуется или не требуется шифровать/расшифровывать блок данных
    при записи/считывании.
*/
enum {
    NONEED = 0, ///< Не требуется
    NEED = 1    ///< Требуется
};

/**
    Звено списка блоков файла
*/
typedef struct dev_blk_info {
    unsigned                cid;    ///< Порядковый номер кластера блоков
    unsigned char           bid;    ///< Порядковый номер блока внутри кластера
    unsigned                num;    ///< Логический порядковый номер в последовательности блоков
    struct dev_blk_info     *next;  ///< Указательн на следующее звено списка
} dev_blk_info_t;

/**
    Уничтожает список блоков
    @param [in] i указатель на голову списка блоков
*/
void
dev_destr_blk_info(dev_blk_info_t *i);

/**
    Выполняет необходимые процедуры по монтированию устройства и первичной
    инициализации драйвера ФС
    @param [in] args указатель на структуру с основными данными ФС
    @return 0 - успешно, -1 - ошибка
*/
int
dev_mount(fcfs_args_t *args);

/**
    Чтение таблицы блоков кластера
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] id порядковый номер кластера
    @return указатель на структуру с информацией о блоках кластера
*/
fcfs_block_list_t *
dev_read_ctable(fcfs_args_t *args, int id);

/**
    Запись таблицы блоков кластера
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] id порядковый номер кластера
    @param [in] bl указатель на структуру с информацией о боках кластера
    @return 0 - успешно, -1 - ошибка
*/
int
dev_write_ctable(fcfs_args_t *args, int id, fcfs_block_list_t *bl);

/**
    Чтение одного логического блока с носителя
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] сid порядковый номер кластера
    @param [in] bid порядковый номер блока в кластере
    @param [in] need_crypt флаг необходимости расшифровки считанных данных
    @return указатель на буффер с данными размера в один логический блок
*/
char *
dev_read_block(fcfs_args_t *args, int cid, int bid, unsigned char need_crypt);

/**
    Чтение директории
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid файловый идентификатор директории
    @param [out] ret_sz длина массива записей директории
    @return NULL - ошибка, иначе указатель на массив записей директорий
*/
fcfs_dir_entry_t *
dev_read_dir(fcfs_args_t *args, int fid, int *ret_sz);

/**
    Запись содержимого директории
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid файловый идентификатор директории
    @param [in] ent указатель на массив записей директории
    @param [in] len длина массива
    @return 0 - успешно, -1 - ошибка
*/
int
dev_write_dir(fcfs_args_t *args, int fid, fcfs_dir_entry_t *ent, int len);

/**
    Поиск свободного идентификатора файла
    @param [in] args указатель на структуру с основными данными ФС
    @return -1 - ошибка, иначе свободный идентификатор
*/
int
dev_free_fid(fcfs_args_t *args);

/**
    Запись заголовка файловой системы (Не реализовано!)
    @param [in] args указатель на структуру с основными данными ФС
    @return 0 - успешно, -1 - ошибка
*/
int
dev_write_head(fcfs_args_t *args);

/**
    Запись битовой карты
    @param [in] args указатель на структуру с основными данными ФС
    @return 0 - успешно, -1 - ошибка
*/
int
dev_write_bitmap(fcfs_args_t *args);

/**
    Запись файловой таблицы
    @param [in] args указатель на структуру с основными данными ФС
    @return 0 - успешно, -1 - ошибка
*/
int
dev_write_table(fcfs_args_t *args);

/**
    Выделение первого блока под файл и установка новых значений в файловой
    таблице
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid файловый идентификатор
    @return 0 - успешно, -1 - ошибка
*/
int
dev_file_alloc(fcfs_args_t *args, int fid);

/**
    Установка размера файла в 0 байт
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid файловый идентификатор
    @return 0 - успешно, -1 - ошибка
*/
int
dev_init_file(fcfs_args_t *args, int fid);

int
dev_file_size(fcfs_args_t *args, int fid);

int
dev_rm_file(fcfs_args_t *args, int fid, int pfid);

int
dev_rm_from_dir(fcfs_args_t *args, int fid, int del_id);

char
dev_upd_bitmap(fcfs_args_t *args, fcfs_block_list_t *bl, int cid);

int *
dev_get_blocks(fcfs_block_list_t *blist, int fid, int *ret_sz);

int
dev_free_cluster(fcfs_args_t *args);

int
dev_write_block(fcfs_args_t *args, int cid, int bid, char *data, int len, unsigned char need_crypt);

int
dev_create_file(fcfs_args_t *args, int pfid, int fid, const char *name, mode_t mode);

dev_blk_info_t *
dev_get_file_seq(fcfs_args_t *args, int fid, int *size);

dev_blk_info_t *
dev_free_blocks(fcfs_args_t *args, int count, int *size);

int
dev_del_block(fcfs_args_t *agrs, int fid, int cid, int bid);

int
dev_read_by_id(fcfs_args_t *args, int fid, int id, char *buf, int lblk_sz,
    dev_blk_info_t *inf, int seq_sz);

int
dev_write_by_id(fcfs_args_t *args, int fid, int id, char *buf,
    int lblk_sz, dev_blk_info_t *inf, int seq_sz);

int
dev_file_reserve(fcfs_args_t *args, int fid, dev_blk_info_t *inf, int seq_sz, int last_num);

int
dev_set_file_size(fcfs_args_t *args, int fid, int size);

int
dev_extd_blk_list(fcfs_args_t *args, dev_blk_info_t **inf, int seq_sz,int count, int fid);

int
dev_free_cluster_from(fcfs_args_t *args, int cid);

int
dev_clust_claim(fcfs_args_t *args, int cid);

int
dev_full_free_cluster(fcfs_args_t *args);

//fs table
int
dev_tbl_clrs_cnt(fcfs_args_t *args, int fid);

int
dev_tbl_clrs_get(fcfs_args_t *args, int fid, int id);

int
dev_tbl_clrs_add(fcfs_args_t *args, int fid, int id);

int
dev_tbl_clrs_set(fcfs_args_t *args, int fid, int id, int val);

#endif
