#pragma once

#include "plugins/ILightweightPlugin.hpp"
#include "dataManagement/DataConnector.hpp"

namespace picongpu
{
namespace MPIStressTest
{

using namespace PMacc;

class TestClass
{
    public:
        void update( MappingDesc *cellDescription )
        {
            const SubGrid<simDim>& subGrid = Environment< simDim >::get().SubGrid();
            DataConnector &dc = Environment< simDim >::get().DataConnector(); 
            FieldE * pField = &(dc.getData< FieldE > (FieldE::getName(), true));
            DataSpace< simDim > guarding = SuperCellSize::toRT() * cellDescription->getGuardingSuperCells();
            //printf("get device data box\n");
            FieldE::DataBoxType dataBox = pField->getDeviceDataBox();
            //printf("done\n");
            FieldE::DataBoxType shifted = dataBox.shift( guarding );
        }
};

class MPIStressTestPlugin : public ILightweightPlugin
{
public:
    MPIStressTestPlugin() :
        cellDescription(NULL)
    {
        Environment<>::get().PluginConnector().registerPlugin(this);
    }

    std::string pluginGetName() const
    {
        return "MPIStressTestPlugin";
    }

    void notify(uint32_t currentStep)
    {
        testClass.update( cellDescription );
        //Some sample MPI magic
        int numProc;
        MPI_Comm_size(mpi_world, &numProc);
        float buffer[numProc];
        float my_value = float(rand());
        if (mode & 1)
            MPI_Allgather( &my_value, 1, MPI_FLOAT, buffer, 1, MPI_FLOAT, mpi_world);
        if (mode & 2)
            MPI_Bcast( buffer, numProc, MPI_FLOAT, 0, mpi_world);
    }

    void pluginRegisterHelp(po::options_description& desc)
    {
        desc.add_options()
            ("mpitest.period", po::value< uint32_t > (&notifyPeriod)->default_value(0),
             "Enable MPIStressTestPlugin [for each n-th step].")
            ("mpitest.mode", po::value< uint32_t > (&mode)->default_value(0),
             "Choose MPIStressTestPlugin mode.");
    }

    void setMappingDescription(MappingDesc *cellDescription)
    {
        printf("Celldescription call\n");
        this->cellDescription = cellDescription;
    }

private:
    MappingDesc *cellDescription;
    TestClass testClass;
    uint32_t notifyPeriod;
    uint32_t mode;
    MPI_Comm mpi_world;

    void pluginLoad()
    {
        //MPI_CHECK(MPI_Comm_dup(MPI_COMM_WORLD, &mpi_world));
        GridController<simDim>& gc = Environment<simDim>::get().GridController();
        mpi_world = gc.getCommunicator().getMPIComm();
        if (notifyPeriod > 0)
            printf("MPIStressTest activated with mode %i.\n",mode);
        Environment<>::get().PluginConnector().setNotificationPeriod(this, notifyPeriod);
    }

    void pluginUnload()
    {
		//MPI_Comm_free(mpi_world);
    }
};
} //namespace MPIStressTest;
} //namespace picongpu;
