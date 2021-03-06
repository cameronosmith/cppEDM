
#include "Parameter.h"

//----------------------------------------------------------------
// Constructor
// Default arguments in Parameter.h
//----------------------------------------------------------------
Parameters::Parameters(
    Method      method,
    std::string pathIn,
    std::string dataFile,
    std::string pathOut,
    std::string predictFile,    
    std::string lib_str,
    std::string pred_str,
    int         E,
    int         Tp,
    int         knn,
    int         tau,
    float       theta,

    std::string columns_str,
    std::string target_str,
    
    bool        embedded,
    bool        verbose,
    
    std::string SmapFile,
    std::string blockFile,
    std::string jacobian_str,
    
    float       svdSig,
    float       tikhonov,
    float       elasticNet,
    
    int         multi,
    std::string libSizes_str,
    int         sample,
    bool        random,
    int         rseed,
    bool        noNeigh,
    bool        fwdTau
    ) :
    // default variable initialization from parameter arguments
    method           ( method ),
    pathIn           ( pathIn ),
    dataFile         ( dataFile ),
    pathOut          ( pathOut ),
    predictOutputFile( predictFile ),    
    lib_str          ( lib_str ),
    pred_str         ( pred_str ),
    E                ( E ),
    Tp               ( Tp ),
    knn              ( knn ),
    tau              ( tau ),
    theta            ( theta ),
    
    columns_str      ( columns_str ),
    target_str       ( target_str ),
    targetIndex      ( 0 ),

    embedded         ( embedded ),
    verbose          ( verbose ),
    
    SmapOutputFile   ( SmapFile ),
    blockOutputFile  ( blockFile ),

    SVDSignificance  ( svdSig ),
    jacobian_str     ( jacobian_str ),
    TikhonovAlpha    ( tikhonov ),
    ElasticNetAlpha  ( elasticNet ),
    
    MultiviewEnsemble( multi ),
    libSizes_str     ( libSizes_str ),
    subSamples       ( sample ),
    randomLib        ( random ),
    seed             ( rseed ),
    noNeighborLimit  ( noNeigh ),
    forwardTau       ( fwdTau ),
    validated        ( false )
{
    if ( method != Method::None ) {
        Validate();
    }
}

//----------------------------------------------------------------
// Destructor
//----------------------------------------------------------------
Parameters::~Parameters() {}

//----------------------------------------------------------------
// 
//----------------------------------------------------------------
void Parameters::Load() {}

//----------------------------------------------------------------
// Index offsets, generate library and prediction indices,
// and parameter validation
//----------------------------------------------------------------
void Parameters::Validate() {

    validated = true;

    //--------------------------------------------------------------
    // Generate library indices: Apply zero-offset
    //--------------------------------------------------------------
    if ( lib_str.size() ) {
        std::vector<std::string> lib_vec = SplitString( lib_str, " \t," );
        if ( lib_vec.size() != 2 ) {
            std::string errMsg("Parameters(): library must be two integers.\n");
            throw std::runtime_error( errMsg );
        }
        int lib_start = std::stoi( lib_vec[0] );
        int lib_end   = std::stoi( lib_vec[1] );
        
        library = std::vector<size_t>( lib_end - lib_start + 1 );
        std::iota ( library.begin(), library.end(), lib_start - 1 );
    }

    //--------------------------------------------------------------
    // Generate prediction indices: Apply zero-offset
    //--------------------------------------------------------------
    if ( pred_str.size() ) {
        std::vector<std::string> pred_vec = SplitString( pred_str, " \t," );
        if ( pred_vec.size() != 2 ) {
            std::string errMsg("Parameters(): prediction must be two "
                               "integers.\n");
            throw std::runtime_error( errMsg );
        }
        int pred_start = std::stoi( pred_vec[0] );
        int pred_end   = std::stoi( pred_vec[1] );
        
        prediction = std::vector<size_t>( pred_end - pred_start + 1 );
        std::iota ( prediction.begin(), prediction.end(), pred_start - 1 );
    }
    
#ifdef DEBUG_ALL
    PrintIndices( library, prediction );
#endif

    //--------------------------------------------------------------
    // Convert multi argument parameters from string to vectors
    //--------------------------------------------------------------
    
    // Columns
    // If columns are purely numeric, then populate vector<size_t> columnIndex
    // otherwise fill in vector<string> columnNames
    if ( columns_str.size() ) {
        
        std::vector<std::string> columns_vec = SplitString( columns_str,
                                                            " \t,\n" );
        
        bool onlyDigits = false;
        
        for ( auto ci = columns_vec.begin(); ci != columns_vec.end(); ++ci ) {
            onlyDigits = OnlyDigits( *ci );
        }
        
        if ( onlyDigits ) {
            for ( auto ci =  columns_vec.begin();
                       ci != columns_vec.end(); ++ci ) {
                columnIndex.push_back( std::stoi( *ci ) );
            }
        }
        else {
            columnNames = columns_vec;
        }
    }
    
    // target
    if ( target_str.size() ) {
        bool onlyDigits = OnlyDigits( target_str );
        if ( onlyDigits ) {
            targetIndex = std::stoi( target_str );
        }
        else {
            targetName = target_str;
        }
    }
    
    // Jacobians
    if ( jacobian_str.size() > 0 ) {
        std::vector<std::string> jac_vec = SplitString( jacobian_str, " \t," );
        if ( jac_vec.size() < 2 ) {
            std::string errMsg( "Parameters(): jacobians must be at least "
                                "two integers.\n" );
            throw std::runtime_error( errMsg );
        }
        jacobians = std::vector<size_t>( jac_vec.size() );
        for ( size_t i = 0; i < jac_vec.size(); i++ ) {
            jacobians.push_back( std::stoi( jac_vec[i] ) );
        }
    }

    // CCM librarySizes
    if ( libSizes_str.size() > 0 ) {
        std::vector<std::string> libsize_vec = SplitString(libSizes_str," \t,");
        if ( libsize_vec.size() != 3 ) {
            std::string errMsg( "Parameters(): librarySizes must be three "
                                "integers.\n" );
            throw std::runtime_error( errMsg );
        }
        for ( size_t i = 0; i < libsize_vec.size(); i++ ) {
            librarySizes.push_back( std::stoi( libsize_vec[i] ) );
        }
    }

    //--------------------------------------------------------------------
    // If Simplex and knn not specified, knn set to E+1
    // If S-Map require knn > E + 1, default is all neighbors.
    if ( method == Method::Simplex ) {
        if ( knn < 1 ) {
            knn = E + 1;
            std::stringstream msg;
            msg << "Parameters::Validate(): Set knn = " << knn
                << " (E+1) for Simplex. " << std::endl;
            std::cout << msg.str();
        }
        if ( knn < E + 1 ) {
            std::stringstream errMsg;
            errMsg << "Parameters::Validate(): Simplex knn of " << knn
                   << " is less than E+1 = " << E + 1 << std::endl;
            throw std::runtime_error( errMsg.str() );
        }
    }
    else if ( method == Method::SMap ) {
        if ( knn > 0 ) {
            if ( knn < E + 1 ) {
                std::stringstream errMsg;
                errMsg << "Parameters::Validate() S-Map knn must be at least "
                          " E+1 = " << E + 1 << ".\n";
                throw std::runtime_error( errMsg.str() );
            }
        }
        else {
            // knn = 0
            knn = prediction.size() - Tp;
            std::stringstream msg;
            msg << "Parameters::Validate(): Set knn = " << knn
                << " for SMap. " << std::endl;
            std::cout << msg.str();
        }
        if ( not embedded and columnNames.size() > 1 ) {
            std::string msg( "Parameters::Validate() WARNING:  "
                             "Multivariable S-Map should use "
                             "-e (embedded) data input to ensure "
                             "data/dimension correspondance.\n" );
            std::cout << msg;
        }

        // S-Map coefficient columns for jacobians start at 1 since the 0th
        // column is the S-Map linear prediction bias term
        if ( jacobians.size() > 1 ) {
            std::vector<size_t>::iterator it = std::find( jacobians.begin(),
                                                          jacobians.end(), 0 );
            if ( it != jacobians.end() ) {
                std::string errMsg( "Parameters::Validate() S-Map coefficient "
                             " columns for jacobians can not use column 0.\n");
                throw std::runtime_error( errMsg );
            }
            if ( jacobians.size() % 2 ) {
                std::string errMsg( "Parameters::Validate() S-Map coefficient "
                                  " columns for jacobians must be in pairs.\n");
                throw std::runtime_error( errMsg );                
            }
        }

        // Tikhonov and ElasticNet are mutually exclusive
        if ( TikhonovAlpha and ElasticNetAlpha ) {
            std::string errMsg( "Parameters::Validate() Multiple S-Map solve "
                                "methods specified.  Use one or none of: "
                                "tikhonov,   elasticNet.\n");
            throw std::runtime_error( errMsg );                
        }

        // Very small alphas don't make sense in elastic net
        if ( ElasticNetAlpha < 0.01 ) {
            std::cout << "Parameters::Validate() ElasticNetAlpha too small."
                         " Setting to 0.01.";
            ElasticNetAlpha = 0.01;
        }
        if ( ElasticNetAlpha > 1 ) {
            std::cout << "Parameters::Validate() ElasticNetAlpha too large."
                         " Setting to 1.";
            ElasticNetAlpha = 1;
        }
    }
    else if ( method == Method::Embed ) {
        // no-op
    }
    else {
        throw std::runtime_error( "Parameters::Validate() "
                                  "Prediction method error.\n" );
    }
}

//------------------------------------------------------------------
// Print to ostream
//  @param os: the stream to print to
//  @return  : the stream passed in
//------------------------------------------------------------------
std::ostream& operator<< ( std::ostream &os, Parameters &p ) {

    // print info about the dataframe
    os << "Parameters: -------------------------------------------\n";

    os << "Method: " << ( p.method == Method::Simplex ? "Simplex" : "SMap" )
       << " E=" << p.E << " Tp=" << p.Tp
       << " knn=" << p.knn << " tau=" << p.tau << " theta=" << p.theta
       << std::endl;

    if ( p.columnNames.size() ) {
        os << "Column Names : [ ";
        for ( auto ci = p.columnNames.begin();
              ci != p.columnNames.end(); ++ci ) {
            os << *ci << " ";
        } os << "]" << std::endl;
    }

    if ( p.targetName.size() ) {
        os << "Target: " << p.targetName << std::endl;
    }

    os << "Library: [" << p.library[0] << " : "
       << p.library[ p.library.size() - 1 ] << "]  "
       << "Prediction: [" << p.prediction[0] << " : "
       << p.prediction[ p.prediction.size() - 1 ]
       << "] " << std::endl;

    
    os << "-------------------------------------------------------\n";
    
    return os;
}

#ifdef DEBUG
//------------------------------------------------------------------
// 
//------------------------------------------------------------------
void Parameters::PrintIndices( std::vector<size_t> library,
                               std::vector<size_t> prediction )
{
    std::cout << "Parameters(): library: ";
    for ( auto li = library.begin(); li != library.end(); ++li ) {
        std::cout << *li << " ";
    } std::cout << std::endl;
    std::cout << "Parameters(): prediction: ";
    for ( auto pi = prediction.begin(); pi != prediction.end(); ++pi ) {
        std::cout << *pi << " ";
    } std::cout << std::endl;
}
#endif
