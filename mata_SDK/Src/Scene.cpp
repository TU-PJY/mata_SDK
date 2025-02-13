#include "Scene.h"
#include "ErrorMessage.h"
#include "SoundUtil.h"

Scene scene;

void Scene::InputFrameTime(float ElapsedTime) {
	FrameTime = ElapsedTime;
}

std::string Scene::Mode() {
	return CurrentRunningMode;
}

void Scene::Stop() {
	UpdateActivateCommand = false;
}

void Scene::Resume() {
	if (!ErrorOccured)
		UpdateActivateCommand = true;
}

void Scene::Update() {
	if (!ErrorScreenState && ErrorOccured) {
		soundUtil.StopAllSounds();
		SwitchToErrorScreen();
		UpdateActivateCommand = false;
		ErrorScreenState = true;
	}

	for (int Layer = 0; Layer < SceneLayer; ++Layer) {
		for (auto& Object : ObjectList[Layer]) {
			if (UpdateActivateCommand) {
				if (!Object->DeleteCommand) {
					if (FloatingFocusCommand) {
						if(Object->FloatingCommand)
							Object->UpdateFunc(FrameTime);
					}
					else
						Object->UpdateFunc(FrameTime);
				}
			}

			if (LoopEscapeCommand) 
				return;

			if (Object->DeleteCommand)
				AddLocation(Layer, CurrentReferLocation);

			else if (Object->SwapCommand)
				AddLocation(Layer, CurrentReferLocation);

			++CurrentReferLocation;
		}
		CurrentReferLocation = 0;
	}
}

void Scene::Render() {
	if (LoopEscapeCommand) {
		LoopEscapeCommand = false;
		return;
	}

	for (int i = 0; i < SceneLayer; ++i) {
		for (auto& Object : ObjectList[i]) {
			if (!Object->DeleteCommand) 
				Object->RenderFunc();
		}
	}
}

void Scene::Init(MODE_PTR ModeFunction) {
	ModeFunction();
	for (int Layer = 0; Layer < SceneLayer; ++Layer)
		DeleteLocation[Layer].reserve(DELETE_LOCATION_BUFFER_SIZE);
}

void Scene::SwitchMode(MODE_PTR ModeFunction) {
	ClearAll();

	if (DestructorBuffer)
		DestructorBuffer();

	ModeFunction();

	if (FloatingActivateCommand) {
		FloatingActivateCommand = false;
		FloatingFocusCommand = false;
	}

	LoopEscapeCommand = true;
}

void Scene::RegisterDestructor(MODE_PTR DestructorFunction) {
	DestructorBuffer = DestructorFunction;
}

void Scene::RegisterModeName(std::string ModeName) {
	CurrentRunningMode = ModeName;
}

void Scene::RegisterController(CONTROLLER_PTR Controller, int Type) {
	Controller();
	if (Type == MODE_TYPE_DEFAULT)
		ControllerBuffer = Controller;
}

void Scene::ReleaseDestructor() {
	DestructorBuffer = nullptr;
}

void Scene::StartFloatingMode(MODE_PTR ModeFunction, bool FloatingFocusFlag) {
	if (FloatingActivateCommand)
		return;

	PrevRunningMode = CurrentRunningMode;
	ModeFunction();
	FloatingFocusCommand = FloatingFocusFlag;

	FloatingActivateCommand = true;
}

void Scene::EndFloatingMode() {
	if (!FloatingActivateCommand)  
		return;

	ClearFloatingObject();
	CurrentRunningMode = PrevRunningMode;

	if (ControllerBuffer)  
		ControllerBuffer();

	FloatingActivateCommand = false;
	FloatingFocusCommand = false;
}

void Scene::AddObject(Object* Object, std::string Tag, int AddLayer, int Type1, int Type2) {
	if (AddLayer > SceneLayer)
		return;

	if (Type1 == OBJECT_TYPE_STATIC_SINGLE || Type2 == OBJECT_TYPE_STATIC_SINGLE) {
		if (auto Object = Find(Tag); Object)
			return;
	}

	ObjectList[AddLayer].emplace_back(Object);

	Object->ObjectTag = Tag;
	Object->ObjectLayer = AddLayer;

	if(Type1 == Type2) {
		if(Type1 == OBJECT_TYPE_STATIC || Type1 == OBJECT_TYPE_STATIC_SINGLE)
			Object->StaticCommand = true;

		else if(Type1 == OBJECT_TYPE_FLOATING)
			Object->FloatingCommand = true;

		return;
	}

	else {
		if(Type1 == OBJECT_TYPE_STATIC || Type2 == OBJECT_TYPE_STATIC || Type1 == OBJECT_TYPE_STATIC_SINGLE || Type2 == OBJECT_TYPE_STATIC_SINGLE)
			Object->StaticCommand = true;

		if(Type1 == OBJECT_TYPE_FLOATING || Type2 == OBJECT_TYPE_FLOATING)
			Object->FloatingCommand = true;
	}
}

void Scene::DeleteObject(Object* Object) {
	Object->DeleteCommand = true;
	Object->ObjectTag = "";
}

void Scene::DeleteObject(std::string Tag, int DeleteRange) {
	if (DeleteRange == DELETE_RANGE_SINGLE) {
		for (int Layer = 0; Layer < SceneLayer; ++Layer) {
			for (auto const& Object : ObjectList[Layer]) {
				if (Object->ObjectTag == Tag) {
					Object->DeleteCommand = true;
					Object->ObjectTag = "";
					return;
				}
			}
		}
	}

	else if (DeleteRange == DELETE_RANGE_ALL) {
		for (int Layer = 0; Layer < SceneLayer; ++Layer) {
			for (auto const& Object : ObjectList[Layer]) {
				if (Object->ObjectTag == Tag) {
					Object->DeleteCommand = true;
					Object->ObjectTag = "";
				}
			}
		}
	}
}

void Scene::RegisterInputObjectList(std::vector<Object*>& Vec) {
	InputObjectListPtr = &Vec;
}

void Scene::AddInputObject(Object* Object) {
	if (InputObjectListPtr)
		InputObjectListPtr->emplace_back(Object);
}

void Scene::DeleteInputObject(Object* Object) {
	if (InputObjectListPtr) {
		auto Found = std::find(begin(*InputObjectListPtr), end(*InputObjectListPtr), Object);
		if (Found != end(*InputObjectListPtr))
			InputObjectListPtr->erase(Found);
	}
}

void Scene::SwapLayer(Object* Object, int TargetLayer) {
	Object->SwapCommand = true;
	Object->ObjectLayer = TargetLayer;
}

Object* Scene::Find(std::string Tag) {
	for (int Layer = 0; Layer < SceneLayer; ++Layer) {
		for (auto const& Object : ObjectList[Layer]) {
			if (Object->ObjectTag == Tag)
				return Object;
		}
	}

	return nullptr;
}

Object* Scene::ReverseFind(std::string Tag) {
	for (int Layer = SceneLayer - 1; Layer > 0; --Layer) {
		for (auto Object = ObjectList[Layer].rbegin(); Object != ObjectList[Layer].rend(); ++Object) {
			if ((*Object)->ObjectTag == Tag)
				return *Object;
		}
	}

	return nullptr;
}

Object* Scene::FindMulti(std::string Tag, int SearchLayer, int Index) {
	auto Object = ObjectList[SearchLayer][Index];
	if(Object->ObjectTag == Tag)
		return ObjectList[SearchLayer][Index];
	
	return nullptr;
}

size_t Scene::LayerSize(int TargetLayer) {
	return ObjectList[TargetLayer].size();
}

void Scene::DeleteTag(Object* Object) {
	Object->ObjectTag = "";
}

void Scene::DeleteTag(std::string Tag) {
	auto Object = Find(Tag);
	if (Object)
		Object->ObjectTag = "";
}

void Scene::CompleteCommand() {
	if(!CommandExist)
		return;

	UpdateObjectList();
	CommandExist = false;
}

void Scene::SetErrorScreen(int ErrorType, std::string Value1, std::string Value2) {
	Value1Buffer = Value1;
	Value2Buffer = Value2;
	ErrorTypeBuffer = ErrorType;
	ErrorOccured = true;
}

//////// private ///////////////
void Scene::AddLocation(int Layer, int Position) {
	DeleteLocation[Layer].emplace_back(Position);
	CommandExist = true;
}

void Scene::UpdateObjectList() {
	int Offset{};

	for (int Layer = 0; Layer < SceneLayer; ++Layer) {
		size_t Size = DeleteLocation[Layer].size();
		if (Size == 0)
			continue;

		for (int i = 0; i < Size; ++i) {
			auto Object = begin(ObjectList[Layer]) + DeleteLocation[Layer][i] - Offset;

			if ((*Object)->DeleteCommand) {
				delete* Object;
				*Object = nullptr;
				ObjectList[Layer].erase(Object);
			}

			else if ((*Object)->SwapCommand) {
				auto Ptr = (*Object);
				ObjectList[Ptr->ObjectLayer].emplace_back((*Object));
				ObjectList[Layer].erase(Object);
				Ptr->SwapCommand = false;
			}

			++Offset;
		}

		DeleteLocation[Layer].clear();
		Offset = 0;
	}
}

void Scene::ClearFloatingObject() {
	for (int Layer = 0; Layer < SceneLayer; ++Layer) {
		size_t Size = LayerSize(Layer);
		for (int i = 0; i < Size; ++i) {
			if (!ObjectList[Layer][i]->StaticCommand && ObjectList[Layer][i]->FloatingCommand) {
				ObjectList[Layer][i]->DeleteCommand = true;
				ObjectList[Layer][i]->ObjectTag = "";
				AddLocation(Layer, i);
			}
		}
	}
}

void Scene::ClearAll() {
	for (int Layer = 0; Layer < SceneLayer; ++Layer) {
		size_t Size = LayerSize(Layer);
		for (int i = 0; i < Size; ++i) {
			if (!ObjectList[Layer][i]->StaticCommand) {
				ObjectList[Layer][i]->DeleteCommand = true;
				ObjectList[Layer][i]->ObjectTag = "";
				AddLocation(Layer, i);
			}
		}
	}
}

void Scene::SwitchToErrorScreen() {
	ClearAll();

	if (Value2Buffer.empty())
		AddObject(new ErrorMessage(ErrorTypeBuffer, Value1Buffer), "error_message", LAYER1);
	else
		AddObject(new ErrorMessage(ErrorTypeBuffer, Value1Buffer, Value2Buffer), "error_message", LAYER1);
}
