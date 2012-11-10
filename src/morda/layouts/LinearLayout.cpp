#include <ting/Array.hpp>
#include <ting/util.hpp>
#include <setjmp.h>

#include "LinearLayout.hpp"

#include "../util/Gravity.hpp"
#include "../util/LeftTopRightBottom.hpp"
#include "../widgets/Padded.hpp"



using namespace morda;



LinearLayout::LinearLayout(const stob::Node& description){
	this->isVertical = true;
	if(const stob::Node* n = description.GetProperty("orientation")){
		if(*n == "horizontal"){
			this->isVertical = false;
		}
	}
	
	this->isReverse = false;
	if(const stob::Node* n = description.GetProperty("reverse")){
		if(n->AsBool()){
			this->isReverse = true;
		}
	}
}



namespace{

const char* D_Margins = "margins";
const char* D_Weight = "weight";

class Info{
public:
	float weight;
	Vec2f dim;
	float margin;//actual margin between child widgets
	Gravity gravity;
	LeftTopRightBottom margins;
};

}//~namespace



//override
void LinearLayout::ArrangeWidgets(Container& cont)const{
	unsigned longIndex = this->isVertical ? 1 : 0;
	unsigned transIndex = this->isVertical ? 0 : 1;
	
	ting::Array<Info> info(cont.NumChildren());
	
	//Calculate rigid size, net weight and store weights and margins
	float rigid = cont.Padding()[longIndex] + cont.Padding()[2 + longIndex];
	float netWeight = 0;
	
	{
		Info* i = info.Begin();
		for(const ting::Ref<Widget>* c = &cont.Children(); *c; c = &(*c)->Next(), ++i){
			ASSERT(info.Overlaps(i))

			const stob::Node* layout = Layout::GetLayoutProp((*c)->Prop());

			if(!layout){
				i->dim = (*c)->GetMinDim();
				i->weight = 0;
				i->gravity = Gravity::Default();
				i->margins = LeftTopRightBottom::Default();
			}else{
				if(const stob::Node* weight = layout->GetProperty(D_Weight)){
					i->weight = weight->AsFloat();
					netWeight += i->weight;
				}else{
					i->weight = 0;
				}

				if(const stob::Node* dim = layout->Child(Layout::Dim::D_Dim()).second){
					i->dim = Layout::Dim::FromSTOB(*dim).ForWidget(*(*c));
				}else{
					i->dim = (*c)->GetMinDim();
				}
				
				i->gravity = Gravity::FromLayout(*layout);
				
				if(const stob::Node* margins = layout->Child(D_Margins).second){
					i->margins = LeftTopRightBottom::FromSTOB(*margins);
				}else{
					i->margins = LeftTopRightBottom::Default();
				}
			}
			
			if(i != info.Begin()){//if not first child
				i->margin = std::max(
						(i - 1)->margins[this->isReverse ? longIndex : (longIndex + 2)],
						i->margins[this->isReverse ? (longIndex + 2) : longIndex]
					);
			}else{
				i->margin = 0;
			}
			
			rigid += i->dim[longIndex] + i->margin;
		}
	}
	
	{
		float flexible = cont.Rect().d[longIndex] - rigid;
		ting::util::ClampBottom(flexible, 0.0f);
		ASSERT(flexible >= 0)
		
		float pos = cont.Padding()[this->isReverse ? (longIndex + 2) : longIndex];//start arranging widgets from padding
		Info *i = info.Begin();
		for(const ting::Ref<Widget>* c = &cont.Children(); *c; c = &(*c)->Next(), ++i){
			Vec2f newSize(i->dim);
			
			if(netWeight > 0){
				newSize[longIndex] += (i->weight / netWeight) * flexible;
			}

			Vec2f newPos;
			if((this->isVertical && !this->isReverse) || (!this->isVertical && this->isReverse)){
				newPos[longIndex] = cont.Rect().d[longIndex] - pos - i->margin - newSize[longIndex];
			}else{
				newPos[longIndex] = pos + i->margin;
			}
			pos += i->margin + newSize[longIndex];
				
			
			//apply gravity
			switch(i->gravity[transIndex]){
				case Gravity::LEFT:
	//			case Gravity::BOTTOM:
					newPos[transIndex] = cont.Padding()[this->isVertical ? 0 : 3];
					break;
				case Gravity::RIGHT:
	//			case Gravity::TOP:
					newPos[transIndex] = cont.Rect().d[transIndex] - newSize[transIndex] - cont.Padding()[this->isVertical ? 2 : 1];
					break;
				default:
				case Gravity::CENTER:
					newPos[transIndex] = (cont.Rect().d[transIndex] - newSize[transIndex]) / 2;//TODO: padding?
					break;
			}

			newPos[longIndex] = ting::math::Round(newPos[longIndex]);
			newPos[transIndex] = ting::math::Round(newPos[transIndex]);
			
			(*c)->MoveTo(newPos);
			
			newSize[longIndex] = ting::math::Round(newSize[longIndex]);
			newSize[transIndex] = ting::math::Round(newSize[transIndex]);

			(*c)->Resize(newSize);
		}
	}
}



//override
morda::Vec2f LinearLayout::ComputeMinDim(const Container& cont)const throw(){
	unsigned longIndex = this->isVertical ? 1 : 0;
	unsigned transIndex = this->isVertical ? 0 : 1;
	
	morda::Vec2f minDim(0);
	
	float prevMargin = 0;
	
	for(const ting::Ref<const Widget>* c = &cont.Children(); *c; c = &(*c)->Next()){
		const morda::Vec2f& md = (*c)->GetMinDim(); //TODO: not min dim but dim from layout prop
		
		if(minDim[transIndex] < md[transIndex]){
			minDim[transIndex] = md[transIndex];
		}
		minDim[longIndex] += md[longIndex];
		
		LeftTopRightBottom margins = LeftTopRightBottom::Default();
		if((*c)->Prop()){
			if(const stob::Node* layout = (*c)->Prop()->Child(Layout::D_Layout()).second){
				if(const stob::Node* m = layout->Child(D_Margins).second){
					margins = LeftTopRightBottom::FromSTOB(*m);
				}
			}
		}
		
		//margin works for non-first children only
		if((*c)->Prev().IsValid()){//if not first child
			minDim[longIndex] += std::max(
					prevMargin,
					margins[this->isReverse ? (longIndex + 2) : longIndex]
				);
		}
		
		prevMargin = margins[this->isReverse ? longIndex : (longIndex + 2)];
	}
	
	minDim[0] += cont.Padding()[0] + cont.Padding()[2];
	minDim[1] += cont.Padding()[1] + cont.Padding()[3];
	
	return minDim;
}
