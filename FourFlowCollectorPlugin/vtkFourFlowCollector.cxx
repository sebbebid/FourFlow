#include "vtkFourFlowCollector.h"
#include "vtkTemporalStreamTracer.h"
#include "vtkAlgorithmOutput.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkTemporalInterpolatedVelocityField.h"
//#include "vtkTemporalDataSet.h"
#include "vtkOutputWindow.h"
#include "vtkAbstractParticleWriter.h"
#include "vtkToolkits.h" // For VTK_USE_MPI 
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkPolygon.h"
#include "vtkTriangleFilter.h"
#include "vtkMassProperties.h"
#include "vtkCellLocator.h"
#include <sstream>
#include <set>

using namespace std;

vtkStandardNewMacro(vtkFourFlowCollector);

vtkFourFlowCollector::vtkFourFlowCollector() : vtkTableAlgorithm() {
	this->SetNumberOfOutputPorts(1);
	this->SetNumberOfInputPorts(2);
	this->CurrentTimeIndex = 0;
	this->continueExecuting = true;
}

vtkFourFlowCollector::~vtkFourFlowCollector() {
}

int vtkFourFlowCollector::FillInputPortInformation(int port, vtkInformation* info) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");    
	if(port==1)
		info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
	return 1;
}

vector<Point> vtkFourFlowCollector::getPoints(vtkPolyData *polyData) {
	vector<Point> points;
	if(vtkDataArray *pointIds = polyData->GetPointData()->GetArray("ParticleId")) {
		cout << pointIds << " " << pointIds->GetClassName() << " " << pointIds->GetNumberOfTuples() <<endl;
		vtkIntArray *pointIdsInt = (vtkIntArray*)pointIds;
		for(int i = 0; i < pointIdsInt->GetNumberOfTuples(); i++) {
			Point point;
			point.id = pointIdsInt->GetValue(i);
			polyData->GetPoint(i, point.pos);
			points.push_back(point);
		}
	}
	return points;
}

LineCollection vtkFourFlowCollector::getCurrentLines(int currentFrame, vtkPolyData *particlePolyData) {
	if(currentFrame == 0) {
		this->pointBuffer.previousPoints = getPoints(particlePolyData);
		this->pointBuffer.currentPoints = this->pointBuffer.previousPoints;
	}
	else {
		this->pointBuffer.previousPoints = this->pointBuffer.currentPoints;
		this->pointBuffer.currentPoints = getPoints(particlePolyData);
	}
	LineCollection lines;
	for(int i = 0; i < this->pointBuffer.previousPoints.size(); i++) {
		for(int j = 0; j < this->pointBuffer.currentPoints.size(); j++) {
			if(this->pointBuffer.previousPoints[i].id == this->pointBuffer.currentPoints[j].id) {
				Line line;
				line.p1 = this->pointBuffer.previousPoints[i];
				line.p2 = this->pointBuffer.currentPoints[j];
				lines.lines.push_back(line);
			}
		}
	}
	return lines;
}

LineCollection vtkFourFlowCollector::getCollidedLines(LineCollection allLines, vtkPolyData *ringPolygonData) {
	LineCollection coolidedLines;
	if(vtkCell *polygon = ringPolygonData->GetCell(0)) {
		cout << "polygon verts: " << polygon->GetNumberOfPoints() << endl;
		for(int i = 0; i < allLines.lines.size(); i++) {
			double t=0.0;
			double x[3];
			double pcoords[3]={0.0};
			int subid=0;
			if(polygon->IntersectWithLine(allLines.lines[i].p1.pos, allLines.lines[i].p2.pos, 1E-6, t, x, pcoords, subid)) {
				coolidedLines.lines.push_back(allLines.lines[i]);
			}
		}
	}
	return coolidedLines;
}

string intToString(int i) {
	string res;
	std::stringstream out;
	out << i;
	res = out.str();
	return res;
}

int vtkFourFlowCollector::RequestData(vtkInformation *request, vtkInformationVector **inputVector, vtkInformationVector *outputVector) {
	if(this->continueExecuting == false)
		return 1;
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
	vtkTable *table = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
	double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	int size = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	int numberOfSources = inputVector[1]->GetNumberOfInformationObjects();
	cout << "numberOfSources: " << numberOfSources << endl; 

	if(this->CurrentTimeIndex == 0) {
	}
	if(this->CurrentTimeIndex < inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS())) {
		for(int i = 0; i < numberOfSources; i++) {
			string name = string("RingPolygon")+intToString(i)+string(": ")+intToString(vtkPolyData::SafeDownCast(inputVector[1]->GetInformationObject(i)->Get(vtkDataObject::DATA_OBJECT()))->GetNumberOfPoints())+string(" points");
			if(!table->GetColumnByName(name.c_str())) {
				vtkFloatArray *arr = vtkFloatArray::New();
				arr->SetName(name.c_str());
				table->AddColumn(arr);
			}
		}
		table->SetNumberOfRows(size);
		vtkPolyData *particlePolyData = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
		LineCollection allLines = this->getCurrentLines(this->CurrentTimeIndex, particlePolyData);
		std::cout << "number of lines: " << allLines.lines.size() << std::endl;
		for(int i = 0; i < numberOfSources; i++) {
			vtkPolyData *ringPolygonData = vtkPolyData::SafeDownCast(inputVector[1]->GetInformationObject(i)->Get(vtkDataObject::DATA_OBJECT()));
			//cout << "---Classname: " << ringPolygonData->GetProducerPort()->GetProducer()->GetClassName() << endl;
			int points = this->getCollidedLines(allLines, ringPolygonData).lines.size();
			table->SetValue(this->CurrentTimeIndex, i, points);
		}

		request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
		this->continueExecuting = true;
		double progress = static_cast<double>(this->CurrentTimeIndex)/static_cast<double>(size);
		std::cout << "progress: " << progress << std::endl;
		this->UpdateProgress(progress);
		this->CurrentTimeIndex++;
	}
	else {
		request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
		this->continueExecuting = false;
		//table->Update();
		this->CurrentTimeIndex = 0;
	}

	return 1;
}

int vtkFourFlowCollector::ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector) {
	if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION())) 
		return this->RequestInformation(request, inputVector, outputVector);       
	if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
      return this->RequestUpdateExtent(request, inputVector, outputVector);
	if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
		return this->RequestData(request, inputVector, outputVector);       
	return 1;
}

int vtkFourFlowCollector::RequestInformation(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector) {
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
		outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
		outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
		this->NumberOfTimeSteps = inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
	else
		this->NumberOfTimeSteps = 0;

	return 1;
}

int vtkFourFlowCollector::RequestUpdateExtent(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector) {
	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
	if(inTimes) {
		inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inTimes[this->CurrentTimeIndex]);
		for(int i = 0; i < inputVector[1]->GetNumberOfInformationObjects(); i++) {
			inputVector[1]->GetInformationObject(i)->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inTimes[this->CurrentTimeIndex]);
		}
	}
	return 1;
}

void vtkFourFlowCollector::AddSourceConnection(vtkAlgorithmOutput* input) {
	this->AddInputConnection(1, input);
}

void vtkFourFlowCollector::RemoveAllSources() {
	this->SetInputConnection(1, 0);
}