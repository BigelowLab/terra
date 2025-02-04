#include <Rcpp.h>
#include "spatRasterMultiple.h"
#include "string_utils.h"

#include "gdal_priv.h"
#include "gdalio.h"
#include "ogr_spatialref.h"

#define GEOS_USE_ONLY_R_API
#include <geos_c.h>


#if GDAL_VERSION_MAJOR >= 3
#include "proj.h"
#define projh
#if PROJ_VERSION_MAJOR > 7
# define PROJ_71
#else
# if PROJ_VERSION_MAJOR == 7
#  if PROJ_VERSION_MINOR >= 1
#   define PROJ_71
#  endif
# endif
#endif
#else
#include <proj_api.h>
#endif

//from sf

#ifdef projh

// [[Rcpp::export]]
std::string proj_version() {
	std::stringstream buffer;
	buffer << PROJ_VERSION_MAJOR << "." << PROJ_VERSION_MINOR << "." << PROJ_VERSION_PATCH;
	return buffer.str();
}

#else

std::string proj_version() {
	int v = PJ_VERSION;
	std::stringstream buffer;
	buffer << v / 100 << "." << (v / 10) % 10 << "." << v % 10;
	return buffer.str();
}

#endif


// [[Rcpp::export]]
std::vector<unsigned char> hex2rgb(std::string s) {
	unsigned char r, g, b;
	s = s.erase(0,1); // remove the "#"
	sscanf(s.c_str(), "%02hhx%02hhx%02hhx", &r, &g, &b);
	std::vector<unsigned char> x = {r, g, b};
	return x;
}

// [[Rcpp::export]]
std::string rgb2hex(std::vector<unsigned char> x) {
	std::stringstream ss;
	ss << "#" << std::hex << std::setw(6) << (x[0] << 16 | x[1] << 8 | x[2] );
	std::string s = ss.str();
	//std::transform(s.begin(), s.end(), s.begin(), ::toupper);
	str_replace_all(s, " ", "0");
	return s;
}


// [[Rcpp::export(name = ".sameSRS")]]
bool sameSRS(std::string x, std::string y) {
	std::string msg;
	SpatSRS srs;
	if (!srs.set(x, msg)) return false;
	return srs.is_same(y, false);
}


// [[Rcpp::export(name = ".SRSinfo")]]
std::vector<std::string> getCRSname(std::string s) {
	OGRSpatialReference x;
	OGRErr erro = x.SetFromUserInput(s.c_str());
	if (erro != OGRERR_NONE) {
		return {"unknown", "", "", "", ""};
	}
	std::string node;
	if (x.IsGeographic()) {
		node = "geogcs";
	} else {
		node = "projcs";
	}

	const char *value;
	std::string name = "";
	value = x.GetAttrValue(node.c_str());
	if (value != NULL) {
		name = value;
	}
	std::string aname = "";
	value = x.GetAuthorityName(node.c_str());
	if (value != NULL) {
		aname = value;
	}

	std::string acode = "";
	value = x.GetAuthorityCode(node.c_str());
	if (value != NULL) {
		acode = value;
	}

	double west, south, east, north;
	west = -10000;
	east = -10000;
	south = -10000;
	north = -10000;

	std::string aoi="", box="";
	#if GDAL_VERSION_MAJOR >= 3
	if (x.GetAreaOfUse(&west, &south, &east, &north, &value)) {
		if (value != NULL) {
			if (west > -1000) {
				aoi	= value;
				box = std::to_string(west) + ", " + std::to_string(east) + ", " + std::to_string(north) + ", " + std::to_string(south);
			}
		}
	}
	#endif
	return {name, aname, acode, aoi, box};
}

// [[Rcpp::export(name = ".getLinearUnits")]]
double getLinearUnits(std::string s) {
	std::string msg;
	SpatSRS srs;
	if (!srs.set(s, msg)) return NAN;
	return srs.to_meter();
}



// [[Rcpp::export(name = ".geotransform")]]
std::vector<double> geotransform(std::string fname) {
	std::vector<double> out;
    GDALDataset *poDataset = static_cast<GDALDataset*>(GDALOpenEx( fname.c_str(), GDAL_OF_RASTER | GDAL_OF_READONLY, NULL, NULL, NULL ));

    if( poDataset == NULL )  {
		Rcpp::Rcout << "cannot read from: " + fname << std::endl;
		return out;
	}

	double gt[6];
	if( poDataset->GetGeoTransform( gt ) != CE_None ) {
		Rcpp::Rcout << "bad geotransform" << std::endl;
	}
	out = std::vector<double>(std::begin(gt), std::end(gt));
	GDALClose( (GDALDatasetH) poDataset );

	return out;
}

// [[Rcpp::export(name = ".gdal_setconfig")]]
void gdal_setconfig(std::string option, std::string value) {
	if (value == "") {
		CPLSetConfigOption(option.c_str(), NULL);
	} else {
		CPLSetConfigOption(option.c_str(), value.c_str());
	}
}

// [[Rcpp::export(name = ".gdal_getconfig")]]
std::string gdal_getconfig(std::string option) {
	const char * value = CPLGetConfigOption(option.c_str(), NULL);
	std::string out = "";
	if (value != NULL) {
		out = value;
	}
	return out;
}


// [[Rcpp::export(name = ".gdalinfo")]]
std::string ginfo(std::string filename, std::vector<std::string> options, std::vector<std::string> oo) {
	std::string out = gdalinfo(filename, options, oo);
	return out;
}

// [[Rcpp::export(name = ".sdinfo")]]
std::vector<std::vector<std::string>> sd_info(std::string filename) {
	std::vector<std::vector<std::string>> sd = sdinfo(filename);
	return sd;
}

// [[Rcpp::export(name = ".gdal_version")]]
std::string gdal_version() {
	const char* what = "RELEASE_NAME";
	const char* x = GDALVersionInfo(what);
	std::string s = (std::string) x;
	return s;
}

// [[Rcpp::export(name = ".geos_version")]]
std::string geos_version(bool runtime = false, bool capi = false) {
	if (runtime)
		return GEOSversion();
	else {
		if (capi)
			return GEOS_CAPI_VERSION;
		else
			return GEOS_VERSION;
	}
}

// [[Rcpp::export(name = ".metadata")]]
std::vector<std::string> metatdata(std::string filename) {
	std::vector<std::string> m = get_metadata(filename);
	return m;
}

// [[Rcpp::export(name = ".sdsmetadata")]]
std::vector<std::string> sdsmetatdata(std::string filename) {
	std::vector<std::string> m = get_metadata_sds(filename);
	return m;
}




// [[Rcpp::export(name = ".parsedsdsmetadata")]]
std::vector<std::vector<std::string>> sdsmetatdataparsed(std::string filename) {
	std::vector<std::string> m = sdsmetatdata(filename);
	std::vector<std::vector<std::string>> s = parse_metadata_sds(m);
	return s;
}
/*
// [[Rcpp::export(name = ".gdaldrivers")]]
std::vector<std::vector<std::string>> gdal_drivers() {
	size_t n = GetGDALDriverManager()->GetDriverCount();
	std::vector<std::vector<std::string>> s(5);
	for (size_t i=0; i<s.size(); i++) {
		s[i].reserve(n);
	}
    GDALDriver *poDriver;
    char **papszMetadata;
	for (size_t i=0; i<n; i++) {
	    poDriver = GetGDALDriverManager()->GetDriver(i);
		s[0].push_back(poDriver->GetDescription());
		s[4].push_back(poDriver->GetMetadataItem( GDAL_DMD_LONGNAME ) );

		papszMetadata = poDriver->GetMetadata();
		bool rst = CSLFetchBoolean( papszMetadata, GDAL_DCAP_RASTER, FALSE);
		s[1].push_back(std::to_string(rst));
		bool create = CSLFetchBoolean( papszMetadata, GDAL_DCAP_CREATE, FALSE);
		bool copy = CSLFetchBoolean( papszMetadata, GDAL_DCAP_CREATECOPY, FALSE);
		s[2].push_back(std::to_string(create + copy));
		bool vsi = CSLFetchBoolean( papszMetadata, GDAL_DCAP_VIRTUALIO, FALSE);
		s[3].push_back(std::to_string(vsi));

	}
	return s;
}
*/


// [[Rcpp::export(name = ".gdaldrivers")]]
std::vector<std::vector<std::string>> gdal_drivers() {
	size_t n = GetGDALDriverManager()->GetDriverCount();
	std::vector<std::vector<std::string>> s(5, std::vector<std::string>(n));
    GDALDriver *poDriver;
    char **papszMetadata;
	for (size_t i=0; i<n; i++) {
	    poDriver = GetGDALDriverManager()->GetDriver(i);
		const char* ss = poDriver->GetDescription();
		if (ss != NULL ) s[0][i] = ss;
		ss = poDriver->GetMetadataItem( GDAL_DMD_LONGNAME );
		if (ss != NULL ) s[4][i] = ss;

		papszMetadata = poDriver->GetMetadata();
		bool rst = CSLFetchBoolean( papszMetadata, GDAL_DCAP_RASTER, FALSE);
		s[1][i] = std::to_string(rst);
		bool create = CSLFetchBoolean( papszMetadata, GDAL_DCAP_CREATE, FALSE);
		bool copy = CSLFetchBoolean( papszMetadata, GDAL_DCAP_CREATECOPY, FALSE);
		s[2][i] = std::to_string(create + copy);
		bool vsi = CSLFetchBoolean( papszMetadata, GDAL_DCAP_VIRTUALIO, FALSE);
		s[3][i] = std::to_string(vsi);
	}
	return s;
}


template <typename... Args>
inline void warningNoCall(const char* fmt, Args&&... args ) {
    Rf_warningcall(R_NilValue, tfm::format(fmt, std::forward<Args>(args)... ).c_str());
}

template <typename... Args>
inline void NORET stopNoCall(const char* fmt, Args&&... args) {
    throw Rcpp::exception(tfm::format(fmt, std::forward<Args>(args)... ).c_str(), false);
}

static void __err_warning(CPLErr eErrClass, int err_no, const char *msg) {
	switch ( eErrClass ) {
        case 0:
            break;
        case 1:
        case 2:
            warningNoCall("%s (GDAL %d)", msg, err_no);
            break;
        case 3:
            warningNoCall("%s (GDAL error %d)", msg, err_no);
            break;
        case 4:
            stopNoCall("%s (GDAL unrecoverable error %d)", msg, err_no);
            break;
        default:
            warningNoCall("%s (GDAL error class %d, #%d)", msg, eErrClass, err_no);
            break;
    }
    return;
}

static void __err_error(CPLErr eErrClass, int err_no, const char *msg) {
	switch ( eErrClass ) {
        case 0:
        case 1:
        case 2:
            break;
        case 3:
            warningNoCall("%s (GDAL error %d)", msg, err_no);
            break;
        case 4:
            stopNoCall("%s (GDAL unrecoverable error %d)", msg, err_no);
            break;
        default:
            stopNoCall("%s (GDAL unrecoverable error %d)", msg, err_no);
            break;
    }
    return;
}


static void __err_fatal(CPLErr eErrClass, int err_no, const char *msg) {
	switch ( eErrClass ) {
        case 0:
        case 1:
        case 2:
        case 3:
            break;
        case 4:
            stopNoCall("%s (GDAL unrecoverable error %d)", msg, err_no);
            break;
        default:
            break;
    }
    return;
}

static void __err_none(CPLErr eErrClass, int err_no, const char *msg) {
    return;
}



// [[Rcpp::export(name = ".set_gdal_warnings")]]
void set_gdal_warnings(int level) {
	if (level==4) {
		CPLSetErrorHandler((CPLErrorHandler)__err_none);
	} else if (level==1) {
		CPLSetErrorHandler((CPLErrorHandler)__err_warning);
	} else if (level==2) {
		CPLSetErrorHandler((CPLErrorHandler)__err_error);
	} else {
		CPLSetErrorHandler((CPLErrorHandler)__err_fatal);
	}
}


// [[Rcpp::export(name = ".gdalinit")]]
void gdal_init(std::string path) {
	set_gdal_warnings(2);
    GDALAllRegister();
    OGRRegisterAll();
	CPLSetConfigOption("GDAL_MAX_BAND_COUNT", "9999999");
	CPLSetConfigOption("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "YES");

	//GDALregistred = true;
#if GDAL_VERSION_MAJOR >= 3
	if (path != "") {
		const char *cp = path.c_str();
		proj_context_set_search_paths(PJ_DEFAULT_CTX, 1, &cp);
	}
#endif
#ifdef PROJ71
	proj_context_set_enable_network(PJ_DEFAULT_CTX, 1);
#endif
}

// [[Rcpp::export(name = ".precRank")]]
std::vector<double> percRank(std::vector<double> x, std::vector<double> y, double minc, double maxc, int tail) {

	std::vector<double> out;
	out.reserve(y.size());
	size_t nx = x.size();
	for (size_t i=0; i<y.size(); i++) {
		if (std::isnan(y[i]) ) {
			out.push_back( NAN );
		} else if ((y[i] < minc) || (y[i] > maxc )) {
			out.push_back( 0 );
		} else {
			size_t b = 0;
			size_t t = 0;
			for (size_t j=0; j<x.size(); j++) {
				if (y[i] > x[j]) {
					b++;
				} else if (y[i] == x[j]) {
					t++;
				} else {
				// y is sorted, so we need not continue
					break;
				}
			}
			double z = (b + 0.5 * t) / nx;
			if (tail == 1) { // both
				if (z > 0.5) {
					z = 2 * (1 - z);
				} else {
					z = 2 * z;
				}
			} else if (tail == 2) { // high
				if (z < 0.5) {
					z = 1;
				} else {
					z = 2 * (1 - z);
				}
			} else { // if (tail == 3) { // low
				if (z > 0.5) {
					z = 1;
				} else {
					z = 2 * z;
				}
			}
			out.push_back(z);
		}
	}
	return(out);
}


// [[Rcpp::export(name = ".setGDALCacheSizeMB")]]
void setGDALCacheSizeMB(double x) {
  GDALSetCacheMax64(static_cast<int64_t>(x) * 1024 * 1024);
}

// [[Rcpp::export(name = ".getGDALCacheSizeMB")]]
double getGDALCacheSizeMB() {
  return static_cast<double>(GDALGetCacheMax64() / 1024 / 1024);
}

// convert NULL-terminated array of strings to std::vector<std::string>
std::vector<std::string> charpp2vect(char **cp) {
	std::vector<std::string> out;
	if (cp == NULL) return out;
	size_t i=0;
	while (cp[i] != NULL) {
		out.push_back(cp[i]);
		i++;
	}
	return out;
}


// [[Rcpp::export(name = ".get_proj_search_paths")]]
std::vector<std::string> get_proj_search_paths() {
	std::vector<std::string> out;
#if GDAL_VERSION_NUM >= 3000300
	char **cp = OSRGetPROJSearchPaths();
	out = charpp2vect(cp);
	CSLDestroy(cp);
#else
	out.push_back("error: GDAL >= 3.0.3 required");
#endif
	return out;
}




// [[Rcpp::export(name = ".set_proj_search_paths")]]
bool set_proj_search_paths(std::vector<std::string> paths) {
	if (!paths.size()) {
		return false;
	}
#if GDAL_VERSION_NUM >= 3000000
	std::vector <char *> cpaths(paths.size()+1);
	for (size_t i = 0; i < paths.size(); i++) {
		cpaths[i] = (char *) (paths[i].c_str());
	}
	cpaths[cpaths.size()] = NULL;
	OSRSetPROJSearchPaths(cpaths.data());
	return true;
#else
	return false;
#endif
}

// [[Rcpp::export(name = ".PROJ_network")]]
std::string PROJ_network(bool enable, std::string url) {
	std::string s = "";
#ifdef PROJ_71
	if (enable) {
		proj_context_set_enable_network(PJ_DEFAULT_CTX, 1);
		if (url.size() > 5) {
			proj_context_set_url_endpoint(PJ_DEFAULT_CTX, url.c_str());
		}
		s = proj_context_get_url_endpoint(PJ_DEFAULT_CTX);
	} else { // disable:
		proj_context_set_enable_network(PJ_DEFAULT_CTX, 0);
	}
#endif
	return s;
}

