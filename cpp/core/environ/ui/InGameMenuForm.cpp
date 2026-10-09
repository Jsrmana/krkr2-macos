#include "InGameMenuForm.h"
#include <algorithm>
#include "cocos2d/MainScene.h"
#include "ui/UIButton.h"
#include "ui/UIListView.h"
#include "ui/UIText.h"
#include "MenuItemIntf.h"
#include "tjsGlobalStringMap.h"
#include "csd/CsdUIFactory.h"

using namespace cocos2d;
using namespace cocos2d::ui;

TVPInGameMenuForm *TVPInGameMenuForm::create(const std::string &title,
                                             tTJSNI_MenuItem *item) {
    auto *ret = new TVPInGameMenuForm;
    if(!item || !ret->initMenuForm()) {
        delete ret;
        return nullptr;
    }
    ret->autorelease();
    ret->initMenu(title, item);
    return ret;
}

bool TVPInGameMenuForm::initMenuForm() {
    if(!Node::init())
        return false;

    // The legacy Node* initFromFile overload is a no-op. Build and bind
    // this form explicitly before initMenu accesses the list.
    const auto size = TVPMainScene::GetInstance()->getUINodeSize();
    const float scale = TVPMainScene::GetInstance()->getUIScale();
    NaviBar.Root = Csd::createNaviBarWithMenu(
        Size(size.width / scale, 120), scale);
    auto *background = NaviBar.Root->getChildByName("background");
    background->setPosition(Vec2::ZERO);
    NaviBar.Left = background->getChildByName<Button *>("left");
    NaviBar.Right = background->getChildByName<Button *>("right");
    NaviBar.Right->setVisible(false);
    bindHeaderController(background);
    addChild(NaviBar.Root);

    RootNode = Widget::create();
    RootNode->setAnchorPoint(Vec2::ZERO);
    auto *list = ListView::create();
    list->setAnchorPoint(Vec2::ZERO);
    list->setName("list");
    list->setDirection(ScrollView::Direction::VERTICAL);
    list->setBounceEnabled(true);
    list->setTouchEnabled(true);
    list->setItemsMargin(11);
    list->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    list->setBackGroundColor(Color3B(42, 42, 42));
    list->setBackGroundColorOpacity(255);
    RootNode->addChild(list);
    addChild(RootNode);
    bindBodyController(RootNode);
    rearrangeLayout();
    return true;
}

void TVPInGameMenuForm::rearrangeLayout() {
    const auto size = TVPMainScene::GetInstance()->getUINodeSize();
    const float scale = TVPMainScene::GetInstance()->getUIScale();
    setContentSize(size);
    const Size bodySize(size.width / scale,
                        std::max(0.0f, size.height / scale - 120));
    RootNode->setContentSize(bodySize);
    RootNode->setScale(scale);
    _list->setContentSize(bodySize);
    NaviBar.Root->setScale(scale);
    NaviBar.Root->setPosition(0, bodySize.height * scale);
}

void TVPInGameMenuForm::bindHeaderController(const Node *allNodes) {
    _title = allNodes->getChildByName<Button *>("title");
    if(_title)
        _title->setEnabled(false);
}

void TVPInGameMenuForm::bindBodyController(const Node *allNodes) {
    _list = allNodes->getChildByName<ListView *>("list");
    if(NaviBar.Left) {
        NaviBar.Left->addClickEventListener([this](cocos2d::Ref *) {
            TVPMainScene::GetInstance()->popUIForm(this);
        });
    }
}

void TVPInGameMenuForm::initMenu(const std::string &title,
                                 tTJSNI_MenuItem *item) {
    _list->removeAllItems();
    if(_title) {
        if(title.empty()) {
            ttstr caption;
            item->GetCaption(caption);
            _title->setTitleText(caption.AsStdString());
        } else {
            _title->setTitleText(title);
        }
    }

    int count = item->GetChildren().size();
    int idx = 0;
    ttstr seperator = TJS::TJSMapGlobalStringMap(TJS_W("-"));
    for(int i = 0; i < count; ++i) {
        tTJSNI_MenuItem *subitem =
            static_cast<tTJSNI_MenuItem *>(item->GetChildren().at(i));
        if(!subitem->GetVisible())
            continue;
        ttstr caption;
        subitem->GetCaption(caption);
        if(caption.IsEmpty() || caption == TJS_W("+"))
            continue;
        _list->pushBackCustomItem(
            createMenuItem(idx, subitem, caption.AsStdString()));
        if(caption != seperator)
            ++idx;
    }
}

cocos2d::ui::Widget *
TVPInGameMenuForm::createMenuItem(int idx, tTJSNI_MenuItem *item,
                                  const std::string &caption) {
    iPreferenceItem *ret = nullptr;
    const cocos2d::Size &size = _list->getContentSize();
    if(!item->GetChildren().empty()) {
        ret = CreatePreferenceItem<tPreferenceItemSubDir>(idx, size, caption);
        ret->addClickEventListener([=](Ref *) {
            if(auto *form = create(caption, item))
                TVPMainScene::GetInstance()->pushUIForm(form);
        });
    } else if(item->GetGroup() > 0 || item->GetRadio()) {
        auto getter = [=]() -> bool { return item->GetChecked(); };
        auto setter = [=](bool) {
            item->OnClick();
            TVPMainScene::GetInstance()->popAllUIForm();
        };
        ret = CreatePreferenceItem<tPreferenceItemCheckBox>(
            idx, size, caption, [=](tPreferenceItemCheckBox *item) {
                item->_getter = getter;
                item->_setter = setter;
            });
    } else if(item->GetChecked()) {
        auto getter = [=]() -> bool { return item->GetChecked(); };
        auto setter = [=](bool) { item->OnClick(); };
        ret = CreatePreferenceItem<tPreferenceItemCheckBox>(
            idx, size, caption, [=](tPreferenceItemCheckBox *item) {
                item->_getter = getter;
                item->_setter = setter;
            });
    } else if(caption == "-") {
        float w = size.width;
        Widget *sep =
            Csd::createSeperateItem(w, 2.0f, Color4F(0.6f, 0.6f, 0.6f, 1.0f));
        return sep;
    } else {
        ret = CreatePreferenceItem<tPreferenceItemConstant>(idx, size, caption);
        ret->addClickEventListener([=](Ref *) {
            TVPMainScene::GetInstance()->scheduleOnce(
                [c = TVPMainScene::GetInstance()](float) { c->popAllUIForm(); },
                0, "close_menu");
            item->OnClick();
        });
        ret->setTouchEnabled(true);
    }
    // KAG can change enabled while text/scene transitions finish. Do not keep
    // the state captured when the menu first opened for the life of the row.
    // Retain the TJS owner while the UI callbacks refer to its native instance.
    const tTJSVariant owner(item->GetOwnerNoAddRef());
    NodeMap nodes(nullptr, ret);
    auto *label = nodes.findController<Text>("title", false);
    auto refresh = [ret, item, label, owner](float) {
        auto *object = owner.AsObjectNoAddRef();
        bool enabled = object && object->IsValid(
            0, nullptr, nullptr, object) == TJS_S_TRUE;
        for(auto *current = item; enabled && current;
            current = dynamic_cast<tTJSNI_MenuItem *>(current->GetParent()))
            enabled = current->GetEnabled();
        ret->setEnabled(enabled);
        if(label)
            label->setTextColor(enabled ? Color4B::WHITE
                                       : Color4B(128, 128, 128, 255));
    };
    refresh(0);
    ret->schedule(refresh, 0.1f, "menu_enabled");
    return ret;
}

void TVPShowPopMenu(tTJSNI_MenuItem *menu) {
    if(auto *form = TVPInGameMenuForm::create(std::string(), menu))
        TVPMainScene::GetInstance()->pushUIForm(form);
}
