#ifndef _MI_EPTZ_H
#define _MI_EPTZ_H

#ifdef __cplusplus
extern "C" {
#endif

#define MI_EPTZ_MAX_PATH (512) /* max. length of full pathname */
#define MI_EPTZ_MAX_LINE_LEN (512) //for file parse

typedef void* EPTZ_DEV_HANDLE;
typedef void* LDC_BIN_HANDLE;

//=================================================================================
typedef enum _mi_eptz_err
{
    MI_EPTZ_ERR_NONE = 0x200,
    MI_EPTZ_ERR_NOT_INIT,
    MI_EPTZ_ERR_NOT_INIT_LUT,
    MI_EPTZ_ERR_NOT_INIT_WORLD2CAM,
    MI_EPTZ_ERR_NOT_INIT_OUT,
    MI_EPTZ_ERR_INVALID_PARAMETER_EPTZ,
    MI_EPTZ_ERR_INVALID_PARAMETER_DONUT,
    MI_EPTZ_ERR_INVALID_PARAMETER_ERP,
    MI_EPTZ_ERR_INVALID_PARAMETER_MAP_DOWNSAMPLE,
    MI_EPTZ_ERR_OUT_OF_MEMORY,
    MI_EPTZ_ERR_OUT_OF_MEMORY_BIN,
    MI_EPTZ_ERR_IC_CHECK,
    MI_EPTZ_ERR_PARSE_LINE_BUFFER,
    MI_EPTZ_ERR_PARSE_FILE_READ,
    MI_EPTZ_ERR_PARSE_LDC_MODE,
    MI_EPTZ_ERR_PARSE_IMAGE_SIZE,
    MI_EPTZ_ERR_PARSE_IMAGE_CENTER,
    MI_EPTZ_ERR_PARSE_GRIDE_SIZE,
    MI_EPTZ_ERR_PARSE_PAN_LIMIT,
    MI_EPTZ_ERR_PARSE_TILT_LIMIT,
    MI_EPTZ_ERR_PARSE_ZOOM_LIMIT,
    MI_EPTZ_ERR_PARSE_ROTATE_LIMIT,
    MI_EPTZ_ERR_PARSE_RADIUS_LIMIT,
    MI_EPTZ_ERR_PARSE_FILE_IN_NONE,
    MI_EPTZ_ERR_MEM_ALLOCATE_FAIL,
    MI_EPTZ_ERR_MEM_FREE_FAIL,
    MI_EPTZ_ERR_LDC_SPLIT_ITERATIVE_FIND,
    MI_EPTZ_ERR_LDC_BIN_OUT_WRITE
}mi_eptz_err;
//=================================================================================
typedef enum _mi_eptz_path
{
    PATH_IN_FOLDER,
    PATH_OUT_FOLDER,
    PATH_LDC_BIN,
    PATH_SRC_IMAGE, //file name for source image
    PATH_NUM
}mi_eptz_path;
//=================================================================================
typedef enum _mi_LDC_MODE
{
    LDC_MODE_4R_CM,     //4 EPTZ views with ceiling mount mode
    LDC_MODE_4R_WM,     //4 EPTZ views with wall mount mode
    LDC_MODE_1R,        //1 undistorted view with ceiling/desk mount mode
    LDC_MODE_2P_CM,     //2 panorama views with ceiling mount mode
    LDC_MODE_1P_CM,     //1 panorama view with ceiling mount mode
    LDC_MODE_1P_WM,     //1 panorama views with wall mount mode
    LDC_MODE_1O,        //bypass mode
    LDC_MODE_1R_WM,     //1 undistorted view with ceiling/desk mount mode
    LDC_MODE_2P_DM      //2 panorama views with desk mount mode
}mi_LDC_MODE;
//=================================================================================
//config parse
typedef struct _mi_eptz_config_param
{
    char path_name[PATH_NUM][MI_EPTZ_MAX_PATH];   //-i, -o, -b
    int path_name_len[PATH_NUM];
    mi_LDC_MODE ldc_mode;   //-m, (0)4R_CM, (1)4R_WM, (2)1R, (3)2p, (4)1P_CM, (5)1P_WM, (6)1O, (7)1R_WM, (8)2P_DM
    int in_width;   //-s
    int in_height;
    int out_width;
    int out_height;
    int out_width_tile;
    int out_height_tile;
    int in_xc;      //-c
    int in_yc;
    int in_fisheye_radius;
    int grid_size;  //-d
    int pan_min;    //-p
    int pan_max;
    int tilt_min;   //-t
    int tilt_max;
    int zoom_min;   //-z
    int zoom_max;
    int rotate_min; //-r
    int rotate_max;
    int radius_min; //-a
    int radius_max;
}mi_eptz_config_param;
//=================================================================================
//parameters
typedef struct _mi_eptz_para
{
    mi_eptz_config_param* ptconfig_para;
    int view_index;
    int pan;
    int tilt;
    int rotate;
    float zoom;
    float zoom_h; //horizental zoom ratio, only for 1P WM mode
    float zoom_v; //vertical zoom ratio, only for 1P WM mode
    float pi;
    float fc;
    float fov;
    int out_rot;
    int r_inside;
    int r_outside;
    int theta_start;
    int theta_end;
}mi_eptz_para;
//=================================================================================
/*
 * mi_eptz_config_parse
 *   Parse configure file.
 *
 *
 * Parameters:
 *  in
 *   pfile_name: configure file path
 *  out
 *   tconfig_para : parsing result
 *
 * Return:
 *   mi_eptz_err error state
 */
mi_eptz_err mi_eptz_config_parse(char* pfile_name, mi_eptz_config_param* tconfig_para);
//=================================================================================
/*
 * mi_eptz_get_buffer_info
 *   Get working buffer length.
 *
 *
 * Parameters:
 *  in
 *   aptconfig_para: Configure setting which invlove image size and other paramters.
 *
 * Return:
 *   working buffer size
 */
int  mi_eptz_get_buffer_info(mi_eptz_config_param* aptconfig_para);
//=================================================================================
/*
* mi_eptz_buffer_free
*   free ldc binary buffer which be allocated by libeptz.a
*
*
* Parameters:
*  in
*   pldc_bin: binary buffer pointer.
*
* Return:
*   mi_eptz_err error state
*/
mi_eptz_err mi_eptz_buffer_free(LDC_BIN_HANDLE pldc_bin);
//=================================================================================
/*
* mi_eptz_runtime_init
*   Generate bin file.
*
*
* Parameters:
*   in
*    pWorkingBuffer: working buffer
*    working_buf_len: working buffer size
*   out
*    apteptz_para : eptz angle parameters
*
* Return:
*   EPTZ_DEV_HANDLE eptz handle pointer
*/
EPTZ_DEV_HANDLE mi_eptz_runtime_init(unsigned char* pWorkingBuffer, int working_buf_len, mi_eptz_para* apteptz_para);
//=================================================================================
/*
 * mi_eptz_runtime_map_gen
 *   Generate bin file.
 *
 *
 * Parameters:
 *  in
 *   apteptz_handle: eptz handle be deacred by mi_eptz_runtime_init()
 *   aptmi_eptz_para : eptz parameters for user assign
 *  out
 *   pldc_bin : out binary buffer pointer.
 *   aplbin_size : out binary size
 *
 * Return:
 *   mi_eptz_err error state
 */
mi_eptz_err mi_eptz_runtime_map_gen(EPTZ_DEV_HANDLE apteptz_handle, mi_eptz_para* aptmi_eptz_para, LDC_BIN_HANDLE* pldc_bin, int* aplbin_size);
//=================================================================================
/*
* mi_donut_runtime_map_gen
*   Generate bin file.
*
*
* Parameters:
*  in
*   apteptz_handle: donut handle be deacred by mi_eptz_runtime_init()
*   aptmi_eptz_para : donut parameters for user assign
*  out
*   pldc_bin : out binary buffer pointer.
*   aplbin_size : out binary size
*
* Return:
*   mi_eptz_err error state
*/
mi_eptz_err mi_donut_runtime_map_gen(EPTZ_DEV_HANDLE apteptz_handle, mi_eptz_para* aptmi_eptz_para, LDC_BIN_HANDLE* pldc_bin, int* aplbin_size);
//=================================================================================
/*
* mi_erp_runtime_map_gen
*   Generate bin file for equirectangular projection.
*
*
* Parameters:
*  in
*   apteptz_handle: eptz handle be deacred by mi_eptz_runtime_init()
*   aptmi_erp_para : equirectangular parameters for user assign
*  out
*   pldc_bin : out binary buffer pointer.
*   aplbin_size : out binary size
*
* Return:
*   mi_eptz_err error state
*/
mi_eptz_err mi_erp_runtime_map_gen(EPTZ_DEV_HANDLE apteptz_handle, mi_eptz_para* aptmi_erp_para, LDC_BIN_HANDLE* pldc_bin, int* aplbin_size);
//=================================================================================
#ifdef __cplusplus
}
#endif

#endif//#ifdef _MI_EPTZ_H
