#ifndef DEV_H
#define DEV_H

#include "fcfs.h"
#include <fcfs_structs.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
    Обозначает необходимость шифрования или дешифрования блока данных
    при записи или чтении.
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

/**
    Получение размера файла
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid файловый идентификатор
    @return размер -1 - ошибка, иначе размер файла в байтах
*/
int
dev_file_size(fcfs_args_t *args, int fid);

/**
    Удаление файла
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid файловый идентификатор
    @param [in] pfid файловый идентификатор родительской директории
    @return 0 - успешно, -1 - ошибка
*/
int
dev_rm_file(fcfs_args_t *args, int fid, int pfid);

/**
    Удаление записи о файле из директории
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid файловый идентификатор
    @param [in] del_id файловый идентификатор для удаления
    @return 0 - успешно, -1 - ошибка
*/
int
dev_rm_from_dir(fcfs_args_t *args, int fid, int del_id);

/**
    Обновление состояния бита указанного кластера в битовой карте ФС
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] bl файловая таблица кластера
    @param [in] cid порядковый номер кластера
    @return 0 - состояние не изменилось, 1 - состояние изменилось
*/
char
dev_upd_bitmap(fcfs_args_t *args, fcfs_block_list_t *bl, int cid);

/**
    Возвращает массив идентификаторов блоков в кластере принадлежащих указанному
    идентификатору
    @param [in] blist указатель на таблицу кластера
    @param [in] fid файловый идентификатор
    @param [out] ret_sz длина массива идентификаторов блоков
    @return NULL - блоков нет, иначе массив идентификаторов блоков
*/
int *
dev_get_blocks(fcfs_block_list_t *blist, int fid, int *ret_sz);

/**
    Поиск первого незаполненного кластера
    @param [in] args указатель на структуру с основными данными ФС
    @return 0 - не найден, иначе номер кластера
*/
int
dev_free_cluster(fcfs_args_t *args);

/**
    Запись блока данных на носитель
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] cid номер кластера
    @param [in] bid номер блока в кластере
    @param [in] data буфер с данными
    @param [in] len длина буфера
    @param [in] need_crypt требуется ли шифровать данные
    @return 0 - удачно, -1 - ошибка
*/
int
dev_write_block(fcfs_args_t *args, int cid, int bid, char *data, int len, unsigned char need_crypt);

/**
    Создает файл в указанной директории
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] pfid идентификатор директории
    @param [in] fid идентификатор создаваемого файла
    @param [in] name название файла
    @param [in] mode права и тип файла
    @return 0 - успешно, -1 - ошибка
*/
int
dev_create_file(fcfs_args_t *args, int pfid, int fid, const char *name, mode_t mode);

/**
    Получить список блоков файла
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid идентификатор файла
    @param [out] size длина списка
    @return NULL - блоков нет(невероятная ситуация), иначе указатель на первый элемент списка
*/
dev_blk_info_t *
dev_get_file_seq(fcfs_args_t *args, int fid, int *size);

/**
    Получить список свободных блоков
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] count требуемое количество свободных блоков
    @param [out] size длина списка
    @return NULL - блоков не найдено, иначе указатель на первый элемент списка
*/
dev_blk_info_t *
dev_free_blocks(fcfs_args_t *args, int count, int *size);

/**
    Освобождение указанного блока
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid идентификатор файла
    @param [in] cid порядковый номер кластера
    @param [in] bid порядковый номер блока
    @return 0 - успешно, иначе ошибка
*/
int
dev_del_block(fcfs_args_t *agrs, int fid, int cid, int bid);

/**
    Чтение блока принадлежащего файлу по его логическому номеру
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid идентификатор файла
    @param [in] id идентификатор блока
    @param [out] buf буфер
    @param [in] lblk_sz размер логического блока
    @param [in] inf список блоков файла
    @param [in] seq_sz длина списка блоков
    @return 0 - удачно, -1 - ошибка
*/
int
dev_read_by_id(fcfs_args_t *args, int fid, int id, char *buf, int lblk_sz,
    dev_blk_info_t *inf, int seq_sz);

/**
    Запись блока принадлежащего файлу по его логическому номеру
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid идентификатор файла
    @param [in] id идентификатор блока
    @param [in] buf буфер
    @param [in] lblk_sz размер логического блока
    @param [in] inf список блоков файла
    @param [in] seq_sz длина списка блоков
    @return 0 - удачно, -1 - ошибка
*/
int
dev_write_by_id(fcfs_args_t *args, int fid, int id, char *buf,
    int lblk_sz, dev_blk_info_t *inf, int seq_sz);

/**
    Пометка блоков как принадлежащих файлу
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid идентификатор файла
    @param [in] inf список будущих блоков файла
    @param [in] seq_sz длина списка блоков
    @param [in] last_num последний логический номер в существующей последовательности
    @return 0 - удачно, -1 - ошибка
*/
int
dev_file_reserve(fcfs_args_t *args, int fid, dev_blk_info_t *inf, int seq_sz, int last_num);

/**
    Чтение блока принадлежащего файлу по его логическому номеру
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid идентификатор файла
    @param [in] size новый размер файла
    @return 0 - удачно, -1 - ошибка
*/
int
dev_set_file_size(fcfs_args_t *args, int fid, int size);

/**
    Расширяет список блоков указанного файла
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] inf список блоков файла
    @param [in] seq_sz длина списка блоков
    @param [in] count требуемое количество новых блоков
    @param [in] fid идентификатор файла
    @return новый размер списка блоков
*/
int
dev_extd_blk_list(fcfs_args_t *args, dev_blk_info_t **inf, int seq_sz,int count, int fid);

/**
    Поиск первого свободного кластера с указанной позиции
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] cid позиция от которой требуется вести поиск
    @return -1 - ошибка, иначе номер кластера
*/
int
dev_free_cluster_from(fcfs_args_t *args, int cid);

/**
    Помечает кластер как полностью пренадлежащий указанному файлу
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] cid номер кластера
    @return 0 - удачно, -1 - ошибка
*/
int
dev_clust_claim(fcfs_args_t *args, int cid);

/**
    Поиск кластера не занятого ни одним файлом
    @param [in] args указатель на структуру с основными данными ФС
    @return номер найденного кластера
*/
int
dev_full_free_cluster(fcfs_args_t *args);

/**
    Получить количество кластеров, в которых находятся блоки файла
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid идентификатор файла
    @return количество кластеров, в которых находятся блоки файла
*/
int
dev_tbl_clrs_cnt(fcfs_args_t *args, int fid);

/**
    Получить номер кластера с принадлежащеми файлу блоками по его логическому номеру
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid идентификатор файла
    @param [in] id логический номер кластера
    @return -1 - ошибка, иначе номер кластера
*/
int
dev_tbl_clrs_get(fcfs_args_t *args, int fid, int id);

/**
    Добавления номера кластера в список кластеров с блоками файла
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid идентификатор файла
    @param [in] id номер кластера
    @return 0 - удачно, -1 - ошибка
*/
int
dev_tbl_clrs_add(fcfs_args_t *args, int fid, int id);

/**
    Чтение блока принадлежащего файлу по его логическому номеру
    @param [in] args указатель на структуру с основными данными ФС
    @param [in] fid идентификатор файла
    @param [in] id логический идентификатор кластера
    @param [in] val новое значение
    @return 0 - удачно, -1 - ошибка
*/
int
dev_tbl_clrs_set(fcfs_args_t *args, int fid, int id, int val);

#endif
