/* The MIT License:

Copyright (c) 2012-2014 Ivan Gagis <igagis@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

// Home page: http://morda.googlecode.com

/**
 * @author Ivan Gagis <igagis@gmail.com>
 */

#pragma once

#include <string>

#include <ting/Shared.hpp>
#include <ting/util.hpp>

#include "../util/keycodes.hpp"

#include "../util/Matrix4.hpp"
#include "../util/Vector2.hpp"
#include "../util/Rectangle2.hpp"
#include "../util/LayoutParams.hpp"

#include "../config.hpp"

#include <stob/dom.hpp>


namespace morda{



class Container;



class Widget : virtual public ting::Shared{
	friend class Container;
	friend class App;
	
public:
	typedef std::list<std::shared_ptr<Widget>> T_ChildrenList;
	
private:
	Container* parent = nullptr;
	T_ChildrenList::iterator parentIter;
	
	bool isHovered = false;

	bool isVisible;

	bool isEnabled;

	morda::Rect2r rect;
	
	//cached minimal dimensions needed to show widget's contents normally
	mutable morda::Vec2r minDim;
	mutable bool minDimNeedsRecomputing = true;
	
	//clip widgets contents by widget's border if set to true
	bool clip;
public:
	bool Clip()const NOEXCEPT{
		return this->clip;
	}
	
	void SetClip(bool clip)NOEXCEPT{
		this->clip = clip;
	}
	
private:
	//logical ID of the widget
	std::string name;
	
	bool relayoutNeeded = true;
	
	std::unique_ptr<stob::Node> layout;
	
	std::unique_ptr<LayoutParams> layoutParams;
public:
	std::unique_ptr<LayoutParams> ResetLayoutParams(std::unique_ptr<LayoutParams> params = nullptr)NOEXCEPT;
	
	bool NeedsRelayout()const NOEXCEPT{
		return this->relayoutNeeded;
	}
	
	const std::string& Name()const NOEXCEPT{
		return this->name;
	}
	
	const Container* Parent()const NOEXCEPT{
		return this->parent;
	}
	
	Container* Parent()NOEXCEPT{
		return this->parent;
	}
	
	//NOTE: if only parent holds Ref then object may be deleted
	void RemoveFromParent();
	
	bool IsHovered()const NOEXCEPT{
		return this->isHovered;
	}
	
private:
	void SetHovered(bool isHovered){
		if(this->isHovered == isHovered){
			return;
		}
		
		this->isHovered = isHovered;
		this->OnHoverChanged();
	}
	
public:

	const morda::Rect2r& Rect()const NOEXCEPT{
		return this->rect;
	}
	
	void MoveTo(const morda::Vec2r& newPos)NOEXCEPT{
		this->rect.p = newPos;
	}
	
	void MoveBy(const morda::Vec2r& delta)NOEXCEPT{
		this->rect.p += delta;
	}

	void Resize(const morda::Vec2r& newDims);
	
	void ResizeBy(const morda::Vec2r& delta){
		this->Resize(this->Rect().d + delta);
	}

	virtual std::shared_ptr<Widget> FindChildByName(const std::string& name)NOEXCEPT;
	
	template <typename T> std::shared_ptr<T> FindChildByNameAs(const std::string& name)NOEXCEPT{
		return std::dynamic_pointer_cast<T>(this->FindChildByName(name));
	}
	
public:
	Widget(const stob::Node* chain);
	
public:

	virtual ~Widget()NOEXCEPT{}

	virtual void Render(const morda::Matr4r& matrix)const{}
	
private:
	void RenderInternal(const morda::Matr4r& matrix)const;
	
private:
	void OnKeyInternal(bool isDown, EKey keyCode);
	
private:
	bool isFocused = false;
public:
	
	//return true to consume
	virtual bool OnKey(bool isDown, morda::EKey keyCode){
		return false;
	}
	
	void Focus()NOEXCEPT;
	
	void Unfocus()NOEXCEPT;
	
	bool IsFocused()const NOEXCEPT{
		return this->isFocused;
	}
	
	virtual void OnFocusedChanged(){}
	
	enum class EMouseButton{
		LEFT,
		RIGHT,
		MIDDLE,
		WHEEL_UP,
		WHEEL_DOWN,
		
		ENUM_SIZE
	};

	
	
	//return true to consume event
	virtual bool OnMouseButton(bool isDown, const morda::Vec2r& pos, EMouseButton button, unsigned pointerId){
		return false;
	}
	
	//return true to consume event
	virtual bool OnMouseMove(const morda::Vec2r& pos, unsigned pointerId){
		return false;
	}

	virtual void OnHoverChanged(){
//		TRACE(<< "Widget::OnHoverChanged(): this->IsHovered() = " << this->IsHovered() << std::endl)
	}

	virtual void OnResize(){
//		TRACE(<< "Widget::OnResize(): invoked" << std::endl)
	}
	
	const morda::Vec2r& GetMinDim()const{
		if(this->minDimNeedsRecomputing){
			this->minDim = this->ComputeMinDim();
			this->minDimNeedsRecomputing = false;
		}
		return this->minDim;
	}

	virtual morda::Vec2r Measure(const Vec2r& offer)const{
		return offer;
	}
	
protected:
	virtual morda::Vec2r ComputeMinDim()const{
		return morda::Vec2r(0, 0);
	}
	
public:

	void SetRelayoutNeeded()NOEXCEPT;

	void SetVisible(bool visible){
		this->isVisible = visible;
		if(!this->isVisible){
			this->SetHovered(false);
		}
	}
	
	bool IsVisible()const NOEXCEPT{
		return this->isVisible;
	}

	void SetEnabled(bool enable)NOEXCEPT{
		this->isEnabled = enable;
	}
	
	bool IsEnabled()const NOEXCEPT{
		return this->isEnabled;
	}
	
	/**
	 * @brief Check if point is within the widget bounds.
     * @param pos - point to check in widget coordinates.
     * @return true if point is inside of the widget boundaries.
	 * @return false otherwise.
     */
	bool Contains(const morda::Vec2r& pos)const NOEXCEPT{
		return morda::Rect2r(morda::Vec2r(0, 0), this->Rect().d).Overlaps(pos);
	}
	
	
	virtual void OnTopmostChanged(){}
	
	bool IsTopmost()const NOEXCEPT;
	
	void MakeTopmost();
};



}//~namespace
