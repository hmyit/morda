#include "Inflater.hpp"

#include "widgets/core/container/Container.hpp"
#include "widgets/core/container/LinearArea.hpp"
#include "widgets/core/container/Frame.hpp"
#include "widgets/core/container/ScrollArea.hpp"
#include "widgets/core/container/Table.hpp"
#include "widgets/core/container/TableRow.hpp"
#include "widgets/core/container/Overlay.hpp"
#include "widgets/core/container/Margins.hpp"

#include "widgets/core/proxy/KeyProxy.hpp"
#include "widgets/core/proxy/MouseProxy.hpp"
#include "widgets/core/proxy/ResizeProxy.hpp"

#include "Morda.hpp"

#include "util/util.hpp"



using namespace morda;



Inflater::Inflater(){
	this->addWidget<Widget>("Widget");
	this->addWidget<Container>("Container");
	this->addWidget<HorizontalArea>("HorizontalArea");
	this->addWidget<VerticalArea>("VerticalArea");
	this->addWidget<Frame>("Frame");
	this->addWidget<MouseProxy>("MouseProxy");
	this->addWidget<ScrollArea>("ScrollArea");
	this->addWidget<Table>("Table");
	this->addWidget<TableRow>("TableRow");
	this->addWidget<KeyProxy>("KeyProxy");
	this->addWidget<Overlay>("Overlay");
	this->addWidget<ResizeProxy>("ResizeProxy");
	this->addWidget<Margins>("Margins");
}



void Inflater::addWidgetFactory(const std::string& widgetName, std::unique_ptr<WidgetFactory> factory){
	std::pair<T_FactoryMap::iterator, bool> ret = this->widgetFactories.insert(
			std::pair<std::string, std::unique_ptr<Inflater::WidgetFactory> >(
					widgetName,
					std::move(factory)
				)
		);
	if(!ret.second){
		throw Inflater::Exc("Failed registering widget type, widget type with given name is already added");
	}
}



bool Inflater::removeWidget(const std::string& widgetName)noexcept{
	if(this->widgetFactories.erase(widgetName) == 0){
		return false;
	}
	return true;
}



std::shared_ptr<morda::Widget> Inflater::inflate(papki::File& fi) {
	std::unique_ptr<stob::Node> root = this->load(fi);
	ASSERT(root)

	return this->inflate(*root);
}


namespace{
const char* defs_c = "defs";
}

namespace{
//Merges two STOB chains. 
std::unique_ptr<stob::Node> mergeGUIChain(const stob::Node* from, const std::set<std::string>& vars, std::unique_ptr<stob::Node> to){
	if(!to){
		if(!from){
			return nullptr;
		}
		return from->cloneChain();
	}
	
	
	std::unique_ptr<stob::Node> children; //children will be stored in reverse order
	
	for(auto src = from; src; src = src->next()){
		if(!src->isProperty()){
			auto c = src->clone();
			c->setNext(std::move(children));
			children = std::move(c);
			continue;
		}
		
		if(!src->child() || *src == "@"){ //@ means reference to a variable
			//No children means that it is a property value, stop further processing of this chain.
			
			//Check that it is the only node in the chain
			if(src != from || src->next()){
				throw Inflater::Exc("malformed gui script: property with several values encountered");
			}
			return to;
		}
		
		auto dst = to->thisOrNext(src->value()).node();
		if(!dst){
			//there is no same named property in 'to', so just clone property there
			to->insertNext(src->clone());
			continue;
		}
		
		if(!dst->child()){
			continue;//no children means that the property is removed in derived template
		}
		
		dst->setChildren(mergeGUIChain(src->child(), vars, dst->removeChildren()));
	}
	
	//add children in reverse order again, so it will be in normal order in the end
	for(; children;){
		auto c = std::move(children);
		children = c->chopNext();
		
		c->setNext(std::move(to));
		to = std::move(c);
	}
	
	return to;
}
}

const Inflater::WidgetFactory* Inflater::findFactory(const std::string& widgetName) {
	auto i = this->widgetFactories.find(widgetName);

	if(i == this->widgetFactories.end()){
		return nullptr;
	}
	
	return i->second.get();
}


std::shared_ptr<morda::Widget> Inflater::inflate(const stob::Node& chain){
//	TODO:
//	if(!App::inst().thisIsUIThread()){
//		throw Exc("Inflate called not from UI thread");
//	}
	
	const stob::Node* n = &chain;
	for(; n && n->isProperty(); n = n->next()){
		if(*n == defs_c){
			if(n->child()){
				this->pushTemplates(*n->child());
				this->pushVariables(*n->child());
			}
		}else{
			throw Exc("Inflater::Inflate(): unknown declaration encountered before first widget");
		}
	}
	
	if(!n){
		return nullptr;
	}
	
	std::unique_ptr<stob::Node> cloned;
	if(auto t = this->findTemplate(n->value())){
		cloned = utki::makeUnique<stob::Node>(t->t->value());
		cloned->setChildren(mergeGUIChain(t->t->child(), t->vars, n->child() ? n->child()->cloneChain() : nullptr));
		n = cloned.get();
	}
	
	
	auto fac = this->findFactory(n->value());
	
	if(!fac){
		TRACE(<< "Inflater::Inflate(): n->value() = " << n->value() << std::endl)
		std::stringstream ss;
		ss << "Failed to inflate, no matching factory found for requested widget name: " << n->value();
		throw Exc(ss.str());
	}

	bool needPopTemplates = false;
	bool needPopVariables = false;
	utki::ScopeExit scopeExit([this, &needPopTemplates, &needPopVariables](){
		if(needPopTemplates){
			this->popTemplates();
		}
		if(needPopVariables){
			this->popVariables();
		}
	});
	
	if(auto v = n->child(defs_c).node()){
		if(v->child()){
			this->pushTemplates(*v->child());
			needPopTemplates = true;
			this->pushVariables(*v->child());
			needPopVariables = true;
		}
	}
	
	{
		if(cloned){
			cloned = cloned->removeChildren();
		}else{
			if(n->child()){
				cloned = n->child()->cloneChain();
			}
		}
		
		this->substituteVariables(cloned.get());
		
		return fac->create(cloned.get());
	}
}



std::unique_ptr<stob::Node> Inflater::load(papki::File& fi){
	std::unique_ptr<stob::Node> ret = stob::load(fi);
	
	ret = std::move(std::get<0>(resolveIncludes(fi, std::move(ret))));

	return ret;
}

Inflater::Template Inflater::parseTemplate(const stob::Node& chain){
	Template ret;
	
	for(auto n = &chain; n; n = n->next()){
		if(n->isProperty()){
			//possibly variable name
			if(n->child()){
				throw Exc("malformed GUI declaration: template argument name has children");
			}
			ret.vars.insert(n->value());
			continue;
		}
		//template definition
		if(ret.t){
			continue;
		}
		ret.t = n->clone();
	}
	if(!ret.t){
		throw Exc("malformed GUI declaration: template has no definition");
	}
	ASSERT(ret.t)
	
	if(auto t = this->findTemplate(ret.t->value())){
		ret.t->setValue(t->t->value());
		ASSERT(t->t->child())
		ret.t->setChildren(mergeGUIChain(t->t->child(), t->vars, ret.t->removeChildren()));
	}
	
	return ret;
}

void Inflater::pushTemplates(const stob::Node& chain){
	decltype(this->templates)::value_type m;
	
	for(auto c = &chain; c; c = c->next()){
		if(c->isProperty()){
			continue;
		}
		
		if(!c->child()){
			throw Exc("Inflater::pushTemplates(): template name has no children, error.");
		}
		
		if(!m.insert(std::make_pair(c->value(), parseTemplate(c->up()))).second){
			throw Exc("Inflater::PushTemplates(): template name is already defined in given templates chain, error.");
		}
	}
	
	this->templates.push_front(std::move(m));
	
//#ifdef DEBUG
//	TRACE(<< "Templates Stack:" << std::endl)
//	for(auto& i : this->templates){
//		TRACE(<< "\tTemplates:" << std::endl)
//		for(auto& j : i){
//			TRACE(<< "\t\t" << j.first << " = " << j.second->ChainToString() << std::endl)
//		}
//	}
//#endif
}



void Inflater::popTemplates(){
	ASSERT(this->templates.size() != 0)
	this->templates.pop_front();
}



const Inflater::Template* Inflater::findTemplate(const std::string& name)const{
	for(auto& i : this->templates){
		auto r = i.find(name);
		if(r != i.end()){
			return &r->second;
		}
	}
//	TRACE(<< "Inflater::FindTemplate(): template '" << name <<"' not found!!!" << std::endl)
	return nullptr;
}



const stob::Node* Inflater::findVariable(const std::string& name)const{
	for(auto& i : this->variables){
		auto r = i.find(name);
		if(r != i.end()){
			return r->second.get();
		}
	}
	TRACE(<< "Inflater::FindVariable(): variable '" << name <<"' not found!!!" << std::endl)
	return nullptr;
}



void Inflater::popVariables(){
	ASSERT(this->variables.size() != 0)
	this->variables.pop_front();
}

namespace{
void substituteVars(stob::Node* to, const std::function<const stob::Node*(const std::string&)>& findVar){
	if(!to || !findVar){
		return;
	}
	
	for(; to;){
		if(*to == "@"){
			if(!to->child()){
				throw Exc("malformed GUI definition: error: reference to a variable holds no variable name");
			}

			const auto name = to->child();

			if(name->next()){
				throw Exc("malformed GUI definition: reference to variable holds more than one variable name");
			}

			if(name->child()){
				throw Exc("malformed GUI definition: variable name has children");
			}

			if(auto var = findVar(name->value())){
				auto next = to->next();
				to->replace(*var);
				to = next;
				continue;
			}
		}else{
			if(to->child()){
				substituteVars(to->child(), findVar);
			}
		}
		to = to->next();
	}
}
}

void Inflater::pushVariables(const stob::Node& chain){
	decltype(this->variables)::value_type m;
	
	for(auto n = &chain; n; n = n->next()){
		if(!n->isProperty()){
			continue;
		}
		
		auto value = n->cloneChildren();
		
		this->substituteVariables(value.get());
		
		if(!m.insert(
				std::make_pair(n->value(), std::move(value))
			).second)
		{
			throw morda::Exc("Inflater::pushDefinitions(): failed to add variable, variable with same name is already defined in this variables block");
		}
	}
	
	this->variables.push_front(std::move(m));
	
//#ifdef DEBUG
//	TRACE(<< "Variables Stack:" << std::endl)
//	for(auto& i : this->variables){
//		TRACE(<< "\tVariables:" << std::endl)
//		for(auto& j : i){
//			TRACE(<< "\t\t" << j.second.first << "|" << j.first << " = " << j.second.second << std::endl)
//		}
//	}
//#endif
}

void Inflater::substituteVariables(stob::Node* to)const{
	substituteVars(
			to,
			[this](const std::string& name){
				return this->findVariable(name);
			}
		);
}
