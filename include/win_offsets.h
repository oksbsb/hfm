#ifndef __HFM_WIN_OFFSETS_H__
#define __HFM_WIN_OFFSETS_H__

enum win_offsets {
    OBJECT_ATTRIBUTES__OBJECT_NAME,
    OBJECT_ATTRIBUTES__ROOT_DIRECTORY,
    UNICODE_STRING__LENGTH,
    UNICODE_STRING__BUFFER,
    IO_STATUS_BLOCK__INFORMATION,
    IO_STATUS_BLOCK__STATUS,
    FILE_RENAME_INFORMATION__FILE_NAME_LENGTH,
    FILE_RENAME_INFORMATION__FILE_NAME,
    FILE_DISPOSITION_INFORMATION__DELETE_FILE,
    KPCR__PRCB,
    KPCR__PRCB_DATA,
    KPRCB__CURRENT_THREAD,
    KTHREAD__PROCESS,
    HANDLE_TABLE__HANDLE_COUNT,
    HANDLE_TABLE__TABLE_CODE,
    HANDLE_TABLE__ENTRY_OBJECT,
    OBJECT_HEADER__BODY,
    OBJECT_HEADER__TYPE_INDEX,
    OBJECT_HEADER_NAME_INFO__NAME,
    FILE_OBJECT__FILE_NAME,
    FILE_OBJECT__DEVICE_OBJECT,
    FILE_OBJECT__VPB,
    FILE_OBJECT__SECTION_OBJECT_POINTER,
    VPB_VOLUME__LABEL,
    VPB_VOLUME__LABEL_LENGTH,
    EPROCESS__PEB,
    EPROCESS__UNIQUE_PROCESS_ID,
    EPROCESS__ACTIVE_PROCESS_LINKS,
    EPROCESS__OBJECT_TABLE,
    DEVICE_OBJECT__DRIVER_OBJECT,
    DRIVER_OBJECT__DRIVER_NAME,
    DEVICE_OBJECT__VPB,
    PEB__PROCESS_PARAMETERS,
    RTL_USER_PROCESS_PARAMETERS__CURRENT_DIRECTORY,
    RTL_USER_PROCESS_PARAMETERS__IMAGE_PATH_NAME,
    CURDIR__DOS_PATH,
    SECTION_OBJECT_POINTERS__DATA_SECTION_OBJECT,
    SECTION_OBJECT_POINTERS__IMAGE_SECTION_OBJECT,
    SECTION_OBJECT_POINTERS__SHARED_CACHE_MAP,
    OBJECT_DIRECTORY__HASH_BUCKETS,
    OBJECT_DIRECTORY__LOCK,
    OBJECT_DIRECTORY_ENTRY__OBJECT,
    OBJECT_DIRECTORY_ENTRY__CHAIN_LINK,
    OBJECT_SYMBOLIC_LINK__DOS_DEVICE_DRIVE_INDEX,
    OBJECT_SYMBOLIC_LINK__LINK_TARGET,
    CONTROL_AREA__SEGMENT,
    SEGMENT__CONTROL_AREA,
    SEGMENT__SIZE_OF_SEGMENT,
    SEGMENT__TOTAL_NUMBER_OF_PTES,
    SUBSECTION__CONTROL_AREA,
    SUBSECTION__PTES_IN_SUBSECTION,
    SUBSECTION__STARTING_SECTOR,
    SUBSECTION__SUBSECTION_BASE,
    SUBSECTION__NEXT_SUBSECTION,
    WIN_OFFSETS_MAX
};

enum win_sizes {
    HANDLE_TABLE_ENTRY,
    CONTROL_AREA,
    FILE_OBJECT,
    OBJECT_HEADER_NAME_INFO,
    OBJECT_HEADER_CREATOR_INFO,
    WIN_SIZES_MAX
};
#endif
