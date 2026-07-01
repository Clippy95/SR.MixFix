module;
#include "framework.h"
export module widescreen;
import general;
SafetyHookInline fullscreen_borderless_handlerT;
uintptr_t fullscreen_borderless_handler(bool fullscreen, bool borderless) {
	return fullscreen_borderless_handlerT.fastcall<uintptr_t>(fullscreen, true);
}

SafetyHookInline os_create_windowT;

char __fastcall os_create_window_hook(LONG nWidth, LONG nHeight, bool fullscreen, bool borderless, HMONITOR hMonitor) {
	if (!fullscreen)
		borderless = true;
	return os_create_windowT.unsafe_fastcall<char>(nWidth, nHeight, fullscreen, borderless, hMonitor);
}

typedef uintptr_t vint_object_base_vtbl;

struct __declspec(align(4)) vint_critical_resources
{
	char* name;
	int type;
	bool autoload;
};

struct vector2i
{
	int x;
	int y;
};

struct rect_i
{
	vector2i nw;
	vector2i se;
};

struct fl_vector2
{
	float x;
	float y;
};


struct vint_doc_metadata_pair
{
	char name[32];
	char value[32];
};



struct vint_object_base;
struct vint_element_base;
struct vint_document
{
	vint_document* document_next;
	vint_document* document_prev;
	rect_i m_clip_stack_data[8];
	vint_doc_metadata_pair m_metadata[16];
	char m_name[32];
	farray<vint_critical_resources, 16> m_critical_resources;
	float m_time_index;
	int m_ref_count;
	unsigned int m_name_crc;
	unsigned int m_handle;
	unsigned int m_clip_stack_depth;
	int m_num_metadata_pairs;
	int m_document_depth;
	vint_object_base* m_root_object;
	vint_element_base* m_root_element;
	unsigned __int16 m_which_resolution;
	bool m_time_index_paused;
	bool m_ready;
	bool m_is_a_template;
	float m_loaded_width;
	float m_loaded_height;
};


struct __declspec(align(4)) vint_object_base
{
	vint_object_base_vtbl* __vftable /*VFT*/;
	vint_object_base* name_hash_next;
	vint_object_base* name_hash_prev;
	vint_object_base* handle_hash_next;
	vint_object_base* handle_hash_prev;
	vint_object_base* factory_next;
	vint_object_base* factory_prev;
	vint_object_base* first_child;
	vint_object_base* next_sibling;
	vint_object_base* parent_object;
	unsigned int m_handle;
	unsigned int m_name_crc;
	vint_document* m_document;
	bool m_loaded_from_template;
	char m_name[32];
};

struct vint_variant_color
{
	float r;
	float g;
	float b;
};


struct __declspec(align(2)) vint_element_base : vint_object_base
{
	vint_variant_color m_tint;
	fl_vector2 m_anchor;
	fl_vector2 m_offset;
	fl_vector2 m_size;
	fl_vector2 m_scale;
	float m_rotation;
	float m_alpha;
	int m_render_mode;
	int m_auto_offset;
	int m_mouse_depth;
	int m_render_depth;
	bool m_is_mask;
	bool m_is_background;
	bool m_is_visible;
};

SafetyHookInline vint_element_base_renderD{};
float* current_aspect_ratio = nullptr;

#define WIDESCREEN_VALUE (16.f / 9.f)

float ultrawide_get_x_difference()
{
	if (*current_aspect_ratio > WIDESCREEN_VALUE)
	{
		return ((*current_aspect_ratio * 720.f) - 1280) / 2;
	}
	return 0.f;
}

float ultrawide_get_scale_x_difference()
{
	if (*current_aspect_ratio > WIDESCREEN_VALUE)
	{
		return *current_aspect_ratio / (WIDESCREEN_VALUE);
	}
	return 1.f;
}

void __fastcall vint_element_base_render_hook(vint_element_base* thisa, void*, float* a2, uintptr_t* BASE, unsigned int a4)
{
	fl_vector2 old_anchor{};
	fl_vector2 old_scale{};
	bool applied = false;
	if (current_aspect_ratio && *current_aspect_ratio > WIDESCREEN_VALUE) {
		if (std::string_view(thisa->m_name) == "vignettes")
		{
			old_anchor = thisa->m_anchor;
			old_scale = thisa->m_scale;

			thisa->m_anchor.x -= ultrawide_get_x_difference();
			thisa->m_scale.x *= ultrawide_get_scale_x_difference();
			applied = true;
		}
	}
	vint_element_base_renderD.unsafe_thiscall(thisa, a2, BASE, a4);
	if (applied)
	{
		thisa->m_anchor = old_anchor;
		thisa->m_scale = old_scale;
	}
}

class widescreen {
public:
	widescreen() {
		MixFix::onAttach() += []() {
			CIniReader ini;
			if (ini.ReadBoolean("Graphics", "DisableBlackBars", true)) {
#if SRTTR || SRIV_HV
				auto pattern = hook::pattern("C6 05 ? ? ? ? ? 84 C9 74 ? F3 0F 10 05");
#else SRTT
				auto pattern = hook::pattern("0F 57 C0 C6 05 ? ? ? ? ? F3 0F 11 05 ? ? ? ? C7 05");
				Memory::VP::Patch<char>(pattern.get_first(0), 0xC3);
				pattern = hook::pattern("80 7C 24 ? ? C6 05 ? ? ? ? ? 74 ? F3 0F 10 05");
#endif
				if(!pattern.empty())
				Memory::VP::Patch<char>(pattern.get_first(0), 0xC3);
			}
#ifdef SRIV_HV
			auto pattern = hook::pattern("84 C9 74 ? B8");
			if (ini.ReadBoolean("Misc","ForceBorderless", SR4_BUILD) && !pattern.empty()) {
				fullscreen_borderless_handlerT = safetyhook::create_inline(pattern.get_first(), fullscreen_borderless_handler);
			}
			 pattern = hook::pattern("40 55 53 56 57 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 48 8B 5D");

			if (!pattern.empty()) {
				os_create_windowT = safetyhook::create_inline(pattern.get_first(), os_create_window_hook);
			}

#endif
			};

#if SRTT
		auto pattern_s = hook::pattern("81 EC ? ? ? ? 53 8B D9 80 BB ? ? ? ? 00 56");
		if (!pattern_s.empty())
		{
			vint_element_base_renderD = safetyhook::create_inline(pattern_s.get_first(-6), vint_element_base_render_hook);
		}
		pattern_s = hook::pattern("? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 10 0D ? ? ? ? ? ? 0F 5A C0");
		if (!pattern_s.empty())
			current_aspect_ratio = *(float**)pattern_s.get_first(2);
#endif

	}
} widescreen;