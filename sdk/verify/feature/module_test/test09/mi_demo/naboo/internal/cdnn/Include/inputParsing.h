//////////////////////////////////////////////////////////////////////////
//			Copyright (c) 2015 CEVA DSP Inc
//////////////////////////////////////////////////////////////////////////
//	File Name	:	inputParsing.h
//	Version		:
//	Created	By	:	Erez Natan
//	Date		:	02/26/2015
//////////////////////////////////////////////////////////////////////////


#ifndef __CDNN_INPUT_PARSING__
#define __CDNN_INPUT_PARSING__



//=============================================================================
//									INCLUDE
//=============================================================================
#include <stdio.h>
#include <CDNNCommonInterface.h>

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

//using namespace std;


//=============================================================================
//									DEFINES
//=============================================================================

#define CMD_LINE_SIZE				(256*2)
#define CMD_LINE_ARGUMENT_NUMBER	(20)
#define INPUT_FILE_NAME_SIZE		(128)
#define TOTAL_TEST_NUMBER			(1024)


#if defined(X64)
	#define INPUT_CONFIG_FILENAME		"../cfg/InputConfiguration.cfg"
#elif defined(WIN32)
	#define INPUT_CONFIG_FILENAME		"./cfg/InputConfiguration.cfg"
#else
	#ifdef 	CDNN_HOST_APPLICATION
		#define INPUT_CONFIG_FILENAME		"./cfg/InputConfigurationLinux.cfg"
	#else
		#ifdef CDNN_DSP_SIMULATION
			#if defined(XM4)
				#define INPUT_CONFIG_FILENAME		"../cfg/InputConfigurationXM4Sim.cfg"
			#elif defined(XM6)
				#define INPUT_CONFIG_FILENAME		"../cfg/InputConfigurationXM6Sim.cfg"
            #else
                #error "no platform is defined for HW simulation"
			#endif
		#else
			#if defined(XM4)
				#define INPUT_CONFIG_FILENAME		"../cfg/InputConfigurationXM4Emu.cfg"
			#elif defined(XM6)
				#define INPUT_CONFIG_FILENAME		"../cfg/InputConfigurationXM6Emu.cfg"
            #else
                #error "no platform is defined for HW emulation"
			#endif
		#endif
	#endif
#endif

#define STATIC_ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

#define NUMBER_OF_INPUT_ARGUMENT	(2)

/*! \brief
* \ingroup group_input_parsing
*/
typedef enum E_CEVA_APPLICATION {
	E_CEVA_APP_CLASSIFICATION,
	E_CEVA_APP_GENDER,
	E_CEVA_APP_AGE,
	E_CEVA_APP_PEDESTRIANDETECTION,
	E_CEVA_APP_FREESPACE,
	E_CEVA_APP_SEGMENTATION,
	E_CEVA_APP_YOLODETECTION,
	E_CEVA_APP_MULTI_NETWORK,
	E_CEVA_APP_NUMBER
}E_CEVA_APPLICATION;

/*! \brief
* \ingroup group_input_parsing
*/
typedef enum EToolMode {
	E_CEVA_IMAGE_CLASSIFICATION = E_CEVA_APP_NUMBER,
	E_CEVA_ROI_CLASSIFICATION,
	E_CEVA_DIRECTORY_IMAGE_CLASSIFICATION,
	E_CEVA_DIRECTORY_ROI_CLASSIFICATION,
	E_CEVA_SVM_TRAIN,
	E_CEVA_SVM_TEST,
	E_CEVA_SVM_PREDICT
} EToolMode;

/*! \brief Describes whether the network has a single or multiple inputs
* \ingroup group_input_parsing
*/
typedef enum E_NetworkInput{
	E_SINGLE_INPUT,
	E_MULTIPLE_INPUT
}E_NetworkInput;

/*! \brief Defines the scale in which the input image will be multiplied by. In case of .mat, this setting is ignored.
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_INPUT_IMAGE_RAW_SCALE
{
	/*! \brief Sets the input fraction bits to 8. */
	E_CDNN_INPUT_IMAGE_RAW_SCALE_1,
	/*! \brief Sets the input fraction bits to 0. */
	E_CDNN_INPUT_IMAGE_RAW_SCALE_256
};

//=============================================================================
//									TYPES
//=============================================================================

/*! \brief input profiler parameters struct
* \ingroup group_input_parsing
*/
typedef struct InputProfilerParameters_st
{
	/*! \brief profiler file name*/
	char	*psFileName;
	/*! \brief profiling level type*/
	int		eType;
	/*! \brief enable profiler module */
	unsigned char	bDoProfiling;
} InputProfilerParameters_t;

/*! \brief input bandwidth reduction parameters
* \ingroup group_input_parsing
*/
typedef struct BWReductionParameters_st
{
	/*! \brief threshold between 0-1 were 1 is no reduction*/
	float	f32ratio;
	/*! \brief enable bandwidth reduction */
	unsigned char	bDoBWReduction;
} BWReductionParametersParameters_t;

/*! \brief
 * \ingroup group_input_parsing
 */
typedef struct InputTestParameters_st
{
	/*! \brief input statistics struct*/
	InputProfilerParameters_t			InputProfiler;
	/*! \brief input bandwidth reduction parameters struct*/
	BWReductionParametersParameters_t	InputBWReduction;

	/*! \brief network input: either single input or multiple input */
	E_NetworkInput	s32NetworkInput;
	/*! \brief number of folders */
	int		s32NumberOfFolders;
	/*! \brief all input files names as loaded from the folder */
	char	**InputFilesInDirectoryName[2];
	/*! \brief number of images in directory*/
	int		s32NumberOfImagesInFolder[2];
	/*! \brief single input image file name or directory file name in case of prediction or directory or single image inference */
	char	*psInputImageOrDirectoryName;
	/*! \brief path to generator directory */
	char	*psGeneratorDir;
	/*! \brief output result file name*/
	char	*OutputResultsFilename;
	/*! \brief trained network file name to load */
	char	*psNetworkFilename;
	/*! \brief network model to train file name to load */
	char	*psModelFilename;

	/*! \brief in eclipse only, scan the folder for subsequent files (image_0.jpg,image_1.jpg,....)*/
	unsigned char	doIncrementalFileSearch;
	/*! \brief enable debug module */
	unsigned char	doDebugLogging;
	/*! \brief enable print network */
	unsigned char	bPrintNetwork;
	/*! \brief float enable boolean */
	unsigned char	doFloat;
	/*! \brief do generation of the network */
	unsigned char	doGeneration;
	/*! \brief disable DMA while running */
	unsigned char	NoDMA;
	/*! \brief resize input buffer */
	unsigned char	doResizeInput;
	/*! \brief enable result in project */
	unsigned char	bPrintResult;
	/*! \brief enable multi input network */
	unsigned char	bMultiInput;
	/*! \brief enable checking precision of fixed point execution */
	unsigned char	bCheckPrecision;

	/*! \brief run  mode indication of the application */
	int				mode;
	/*! \brief network mode indication */
	int				NetworkMode;
	/*! \brief swap image channels in the for the input layer */
	int				ChannelSwap;
	/*! \brief sets the raw scale */
	int				RawScale;
	/*! \brief the network size after reading it from the file */
	unsigned int	u32NetworkSize;

	char	*positiveDirectory;
	char	*negativeDirectory;
	char	*outputDirectory;



	float		threshold;

	/*! \brief input argument temp buffer */
	char						cmd_line[CMD_LINE_ARGUMENT_NUMBER][CMD_LINE_SIZE];
	/*! \brief number of input arguments in each line */
	unsigned int				argNumber;
	/*! \brief number of input arguments in each line */
	void						*pCDNNNetwork;
	/*! \brief the platform type running the CDNN */
	E_CDNN_USER_DEVICE_TYPE		PlatformType;

} InputTestParameters_t;


/*! \brief
* \ingroup group_input_parsing
*/
typedef struct SToolOptionStruct
{
	const char	*longName;
	const char	shortName;
	int			isRequired;
	int			hasArgument;
	int			numberOfSubArguments;
	const char		*defaultValue[NUMBER_OF_INPUT_ARGUMENT];
	const char		*description;
} SToolOption;


//=============================================================================
//									GLOBAL VARIABLES
//=============================================================================

#ifdef __cplusplus
extern "C"{
#endif
	extern InputTestParameters_t* g_ppApplicationParameters[TOTAL_TEST_NUMBER];
#ifdef __cplusplus
}
#endif

//=============================================================================
//									FUNCTIONS
//=============================================================================

/*! \brief parsing the input parameters
* \param [in] option parameter
* \param [in] pointer to the switch parameter
* \param [in] pointer to the input data
* \ingroup group_input_parsing
*/
int ApplicationParseInputParam(int argc, char* argv, InputTestParameters_t* outValues);

#ifdef __cplusplus
extern "C"{
#endif

	/*! \brief appends an index before the file extension, for example: if src = "file.xls" and idx = 3, dst will be file_3.xls
	* \param [in] option parameter
	* \param [in] pointer to the switch parameter
	* \param [in] pointer to the input data
	* \ingroup group_input_parsing
	*/
	void appendTailIndex(char* src, char* dst, int idx);

#ifdef __cplusplus
}
#endif


#endif // __CDNN_INPUT_PARSING__
