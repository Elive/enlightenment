#ifdef E_TYPEDEFS
typedef struct _E_Import_Config_Dialog E_Import_Config_Dialog;
#else
#ifndef E_IMPORT_CONFIG_DIALOG_H
#define E_IMPORT_CONFIG_DIALOG_H

#define E_IMPORT_CONFIG_DIALOG_TYPE 0xE0b01040

typedef enum _E_Import_Place
{
    IMPORT_WALLPAPER_ALL = 0,
    IMPORT_WALLPAPER_DESK = 1,
    IMPORT_WALLPAPER_SCREEN = 2
} E_Import_Place;

typedef enum _E_Import_Place_Show
{
    IMPORT_PLACE_HIDE = 0,
    IMPORT_PLACE_SHOW = 1
} E_Import_Place_Show;

E_Import_Place_Show e_import_place_show;

struct _E_Import_Config_Dialog
{
   E_Object              e_obj_inherit;
   Ecore_End_Cb          ok;
   Ecore_Cb              cancel;

   const char *file;
   int   method;
   int   external;
   int   quality;
   E_Color              color;

   Ecore_Exe            *exe;
   Ecore_Event_Handler  *exe_handler;
   const char         *path;
   char          *tmpf;
   const char          *fdest;

   E_Dialog             *dia;
   int                  place;
};


EAPI E_Import_Config_Dialog *e_import_config_dialog_show(E_Container *con, const char *path, Ecore_End_Cb ok, Ecore_Cb cancel);

#endif
#endif
