#define IMGUI_DEFINE_MATH_OPERATORS
#include "gui.hpp"

using namespace ImGui;

void c_gui::render_circle_for_horizontal_bar( ImVec2 pos, ImColor color, float alpha ) {

	auto draw = GetWindowDrawList( );
	draw->AddCircleFilled( pos, 6, ImColor( color.Value.x, color.Value.y, color.Value.z, alpha * GetStyle( ).Alpha ) );
}

bool c_gui::tab( const char* icon, const char* label, bool selected ) {

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label );

	auto icon_size = CalcTextSize( icon );
	auto label_size = CalcTextSize( label, 0, 1 );

	auto pos = window->DC.CursorPos;
	auto draw = window->DrawList;

	ImRect bb( pos, ImVec2( pos.x + GetWindowWidth( ), pos.y + 30 ) );
	ItemAdd( bb, id );
	ItemSize( bb, GetStyle( ).FramePadding.y );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	static std::unordered_map < ImGuiID, float > values;
	auto value = values.find( id );
	if ( value == values.end( ) ) {

		values.insert( { id, 0.f } );
		value = values.find( id );
	}

	value->second = ImLerp( value->second, ( selected ? 1.f : 0.f ), 0.05f );

	draw->AddRectFilled( bb.Min, bb.Max, gui.frame_active.to_im_color( 0.5f * value->second ), 5 );

	draw->AddText( ImVec2( bb.Min.x + 10, bb.GetCenter( ).y - label_size.y / 2 ), gui.accent_color.to_im_color( ), icon );
	draw->AddText( ImVec2( bb.Min.x + 35, bb.GetCenter( ).y - label_size.y / 2 ), GetColorU32( ImGuiCol_Text ), label );

	return pressed;
}

bool c_gui::subtab( const char* label, bool selected, int size, ImDrawFlags flags ) {

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label );

	auto label_size = CalcTextSize( label, 0, 1 );

	auto pos = window->DC.CursorPos;
	auto draw = window->DrawList;

	ImRect bb( pos, ImVec2( pos.x + GetWindowWidth( ) / size, pos.y + pos.y - pos.x ) );
	ItemAdd( bb, id );
	ItemSize( bb, GetStyle( ).FramePadding.y );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	static std::unordered_map < ImGuiID, float > values;
	auto value = values.find( id );
	if ( value == values.end( ) ) {

		values.insert( { id, 0.f } );
		value = values.find( id );
	}

	value->second = ImLerp( value->second, ( selected ? 1.f : 0.f ), 0.05f );

	draw->AddRectFilled( bb.Min, bb.Max, gui.frame_active.to_im_color( 0.8f * value->second ), 4, flags );

	draw->AddText( bb.GetCenter( ) - label_size / 2, selected ? gui.text.to_im_color( ) : gui.text_disabled.to_im_color( ), label );

	// advance the cursor so tabs are laid out horizontally, not stacked/scroll
	SetCursorPos( ImVec2( GetCursorPosX( ) + GetWindowWidth( ) / size, GetCursorPosY( ) ) );

	return pressed;
}

void c_gui::group_box( const char* name, ImVec2 size_arg ) {

	auto window = GetCurrentWindow( );
	auto pos = window->DC.CursorPos;

	BeginChild( std::string( name ).append( ".main" ).c_str( ), size_arg, false, ImGuiWindowFlags_NoScrollbar );

	GetWindowDrawList( )->AddRectFilled( ImVec2( pos.x, pos.y + 20 ), ImVec2( pos.x + size_arg.x, pos.y + size_arg.y ), gui.group_box_bg.to_im_color( ), 6 );
	GetWindowDrawList( )->AddRect( ImVec2( pos.x, pos.y + 20 ), ImVec2( pos.x + size_arg.x, pos.y + size_arg.y ), gui.border.to_im_color( ), 6 );

	GetWindowDrawList( )->AddText( ImVec2( pos.x + 12, pos.y ), GetColorU32( ImGuiCol_Text, 0.5f ), name );

	SetCursorPos( ImVec2( 12, 21 ) );
	PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 10 ) );
	BeginChild( name, ImVec2( size_arg.x - 24, size_arg.y - 21 ), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar );

	BeginGroup( );

	PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 8, 10 ) );
	PushStyleVar( ImGuiStyleVar_Alpha, gui.m_anim );
}

void c_gui::end_group_box( ) {

	PopStyleVar( 3 );
	EndGroup( );
	EndChild( );
	EndChild( );
}