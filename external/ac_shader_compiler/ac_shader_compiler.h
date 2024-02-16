#pragma once

#include <ac/ac_core.h>
#include <string.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

#define AC_MAX_PUSH_CONSTANT_RANGE 16
#define AC_MAX_REGISTER 1000
#define AC_CBV_OFFSET 0
#define AC_SRV_OFFSET (AC_MAX_REGISTER)
#define AC_UAV_OFFSET (AC_MAX_REGISTER * 2)
#define AC_SAMPLER_OFFSET (AC_MAX_REGISTER * 3)
#define AC_SHADER_SPACE_COUNT 3

typedef uint8_t  ac_shader_space;
typedef uint32_t ac_shader_register;
typedef uint32_t ac_shader_binding_count;
typedef uint32_t ac_shader_descriptor_count;
typedef uint8_t  ac_shader_location;

typedef uint8_t ac_shader_space_count;
typedef uint8_t ac_shader_attribute_count;

typedef enum ac_shader_bytecode {
  ac_shader_bytecode_unknown = 0,
  ac_shader_bytecode_reflection = 1,
  ac_shader_bytecode_dxil = 2,
  ac_shader_bytecode_spirv = 3,
  ac_shader_bytecode_metallib = 4,
  ac_shader_bytecode_orbis = 5,
  ac_shader_bytecode_prospero = 6,
  ac_shader_bytecode_nintendo = 7,
  ac_shader_bytecode_count
} ac_shader_bytecode;

typedef enum ac_shader_compiler_platform {
  ac_shader_compiler_platform_auto = 0,
  ac_shader_compiler_platform_windows = 1,
  ac_shader_compiler_platform_linux = 2,
  ac_shader_compiler_platform_apple_macos = 3,
  ac_shader_compiler_platform_apple_ios = 4,
  ac_shader_compiler_platform_nintendo_switch = 5,
  ac_shader_compiler_platform_ps4 = 6,
  ac_shader_compiler_platform_ps5 = 7,
  ac_shader_compiler_platform_xbox_one = 8,
  ac_shader_compiler_platform_xbox_xs = 9,
} ac_shader_compiler_platform;

typedef enum ac_shader_compiler_stage {
  ac_shader_compiler_stage_vertex = 0,
  ac_shader_compiler_stage_pixel = 1,
  ac_shader_compiler_stage_compute = 2,
  ac_shader_compiler_stage_raygen = 3,
  ac_shader_compiler_stage_any_hit = 4,
  ac_shader_compiler_stage_closest_hit = 5,
  ac_shader_compiler_stage_miss = 6,
  ac_shader_compiler_stage_intersection = 7,
  ac_shader_compiler_stage_amplification = 8,
  ac_shader_compiler_stage_mesh = 9,
  ac_shader_compiler_stage_count,
} ac_shader_compiler_stage;

typedef enum ac_shader_attribute_semantic {
  ac_shader_attribute_semantic_position = 0,
  ac_shader_attribute_semantic_normal = 1,
  ac_shader_attribute_semantic_color = 2,
  ac_shader_attribute_semantic_texcoord0 = 3,
  ac_shader_attribute_semantic_texcoord1 = 4,
  ac_shader_attribute_semantic_texcoord2 = 5,
  ac_shader_attribute_semantic_texcoord3 = 6,
  ac_shader_attribute_semantic_texcoord4 = 7,
  ac_shader_attribute_semantic_texcoord5 = 8,
  ac_shader_attribute_semantic_texcoord6 = 9,
  ac_shader_attribute_semantic_texcoord7 = 10,
  ac_shader_attribute_semantic_texcoord8 = 11,
  ac_shader_attribute_semantic_texcoord9 = 12,
  ac_shader_attribute_semantic_count,
} ac_shader_attribute_semantic;

typedef enum ac_shader_descriptor_type {
  ac_shader_descriptor_type_sampler = 0,
  ac_shader_descriptor_type_srv_image = 1,
  ac_shader_descriptor_type_uav_image = 2,
  ac_shader_descriptor_type_cbv_buffer = 3,
  ac_shader_descriptor_type_srv_buffer = 4,
  ac_shader_descriptor_type_uav_buffer = 5,
  ac_shader_descriptor_type_as = 6,
} ac_shader_descriptor_type;

typedef enum ac_shader_reflection_bit {
  ac_shader_reflection_vertex_input_bit = AC_BIT(0),
  ac_shader_reflection_workgroup_bit = AC_BIT(1),
  ac_shader_reflection_spaces_bit = AC_BIT(2),
  ac_shader_reflection_push_constant_bit = AC_BIT(3),
} ac_shader_reflection_bit;
typedef uint32_t ac_shader_reflection_bits;

typedef struct ac_shader_compiler_static_sampler {
  ac_shader_space space;
  uint32_t        reg;
} ac_shader_compiler_static_sampler;

typedef struct ac_shader_compiler_shader_info {
  ac_shader_compiler_platform              platform;
  ac_shader_compiler_stage                 stage;
  size_t                                   source_size;
  const char*                              source;
  const char*                              input_filename;
  uint32_t                                 permutation_id;
  uint32_t                                 max_permutations;
  const char*                              format;
  const char*                              debug_dir;
  bool                                     enable_spirv;
  uint32_t                                 static_sampler_count;
  const ac_shader_compiler_static_sampler* static_samplers;
} ac_shader_compiler_shader_info;

typedef struct ac_shader_vertex_attribute {
  ac_shader_attribute_semantic semantic;
  ac_shader_location           location;
} ac_shader_vertex_attribute;

typedef struct ac_shader_binding {
  ac_shader_space            space;
  ac_shader_register         reg;
  ac_shader_descriptor_type  type;
  ac_shader_descriptor_count descriptor_count;
} ac_shader_binding;

typedef struct ac_shader_workgroup {
  uint8_t x;
  uint8_t y;
  uint8_t z;
} ac_shader_workgroup;

AC_API ac_result
ac_shader_compiler_create_shader(
  const ac_shader_compiler_shader_info* info,
  uint32_t*                             size,
  void**                                shader);

AC_API void
ac_shader_compiler_destroy_shader(void* shader);

static inline uint32_t
ac_shader_compiler_get_shader_binding_index(
  ac_shader_descriptor_type type,
  ac_shader_register        reg)
{
  switch (type)
  {
  case ac_shader_descriptor_type_cbv_buffer:
    return reg + AC_CBV_OFFSET;
  case ac_shader_descriptor_type_uav_image:
  case ac_shader_descriptor_type_uav_buffer:
    return reg + AC_UAV_OFFSET;
  case ac_shader_descriptor_type_srv_image:
  case ac_shader_descriptor_type_srv_buffer:
  case ac_shader_descriptor_type_as:
    return reg + AC_SRV_OFFSET;
  case ac_shader_descriptor_type_sampler:
    return reg + AC_SAMPLER_OFFSET;
  default:
    break;
  }

  AC_ASSERT(false);

  return UINT32_MAX;
}

static inline const char*
ac_shader_compiler_get_entry_point_name(ac_shader_compiler_stage stage)
{
  switch (stage)
  {
  case ac_shader_compiler_stage_vertex:
    return "vs";
  case ac_shader_compiler_stage_pixel:
    return "fs";
  case ac_shader_compiler_stage_compute:
    return "cs";
  case ac_shader_compiler_stage_raygen:
    return "raygen";
  case ac_shader_compiler_stage_any_hit:
    return "any_hit";
  case ac_shader_compiler_stage_closest_hit:
    return "closest_hit";
  case ac_shader_compiler_stage_miss:
    return "miss";
  case ac_shader_compiler_stage_intersection:
    return "intersection";
  case ac_shader_compiler_stage_amplification:
    return "task";
  case ac_shader_compiler_stage_mesh:
    return "mesh";
  default:
    break;
  }
  AC_ASSERT(false);
  return "";
}

AC_API static inline const void*
ac_shader_compiler_get_bytecode(
  const void*        shader,
  uint32_t*          size,
  ac_shader_bytecode bytecode)
{
  typedef struct ac_shader_header {
    uint32_t blob_count;
    struct {
      ac_shader_bytecode bytecode;
      uint32_t           size;
      uint32_t           offset;
    } so[1];
  } ac_shader_header;

  *size = 0;

  if (!shader)
  {
    return NULL;
  }

  const ac_shader_header* header = (const ac_shader_header*)(shader);

  const uint8_t* p = (const uint8_t*)shader;

  for (uint32_t i = 0; i < header->blob_count; ++i)
  {
    if (header->so[i].bytecode == bytecode)
    {
      *size = header->so[i].size;
      return p + header->so[i].offset;
    }
  }

  return NULL;
}

AC_API static inline bool
ac_shader_compiler_has_push_constant(void* reflection)
{
  uint8_t* code = (uint8_t*)reflection;

  ac_shader_reflection_bits bits;
  memcpy(&bits, code, sizeof(bits));

  return (bits & ac_shader_reflection_push_constant_bit);
}

AC_API static inline ac_result
ac_shader_compiler_get_locations(
  void*               reflection,
  ac_shader_location* locations)
{
  uint8_t* code = (uint8_t*)reflection;

  ac_shader_reflection_bits bits;
  memcpy(&bits, code, sizeof(bits));
  code += sizeof(ac_shader_reflection_bits);

  if (bits & ac_shader_reflection_vertex_input_bit)
  {
    ac_shader_attribute_count count;
    memcpy(&count, code, sizeof(count));
    code += sizeof(ac_shader_attribute_count);
    for (ac_shader_attribute_count a = 0; a < count; ++a)
    {
      ac_shader_vertex_attribute attr;
      memcpy(&attr, code, sizeof(attr));
      code += sizeof(ac_shader_vertex_attribute);
      locations[attr.semantic] = attr.location;
    }
  }

  return ac_result_success;
}

AC_API static inline ac_result
ac_shader_compiler_get_workgroup(void* reflection, ac_shader_workgroup* wg)
{
  AC_ASSERT(reflection);
  AC_ASSERT(wg);

  if (!wg)
  {
    AC_DEBUGBREAK();
    return ac_result_unknown_error;
  }

  uint8_t* code = (uint8_t*)reflection;

  ac_shader_reflection_bits bits;
  memcpy(&bits, code, sizeof(bits));
  code += sizeof(ac_shader_reflection_bits);

  if (bits & ac_shader_reflection_vertex_input_bit)
  {
    ac_shader_attribute_count count;
    memcpy(&count, code, sizeof(count));
    code += sizeof(ac_shader_attribute_count);
    code += count * sizeof(ac_shader_vertex_attribute);
  }

  if (bits & ac_shader_reflection_workgroup_bit)
  {
    memcpy(wg, code, sizeof(ac_shader_workgroup));
    code += sizeof(ac_shader_workgroup);
  }

  if (!wg->x && !wg->y && !wg->z)
  {
    AC_DEBUGBREAK();
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

AC_API static inline ac_result
ac_shader_compiler_get_bindings(
  void*              reflection,
  uint32_t*          out_count,
  ac_shader_binding* out)
{
  const uint8_t* code = (uint8_t*)reflection;

  *out_count = 0;

  if (code == NULL)
  {
    AC_DEBUGBREAK();
    return ac_result_unknown_error;
  }

  ac_shader_reflection_bits bits;
  memcpy(&bits, code, sizeof(bits));
  code += sizeof(ac_shader_reflection_bits);

  if (bits & ac_shader_reflection_vertex_input_bit)
  {
    ac_shader_attribute_count count;
    memcpy(&count, code, sizeof(count));
    code += sizeof(ac_shader_attribute_count);
    code += sizeof(ac_shader_vertex_attribute) * count;
  }

  if (bits & ac_shader_reflection_workgroup_bit)
  {
    code += sizeof(ac_shader_workgroup);
  }

  if (bits & ac_shader_reflection_spaces_bit)
  {
    ac_shader_space_count count;
    memcpy(&count, code, sizeof(count));
    code += sizeof(ac_shader_space_count);

    for (ac_shader_space_count s = 0; s < count; ++s)
    {
      ac_shader_space space;
      memcpy(&space, code, sizeof(space));
      code += sizeof(ac_shader_space);

      ac_shader_binding_count binding_count;
      memcpy(&binding_count, code, sizeof(binding_count));
      code += sizeof(ac_shader_binding_count);
      for (ac_shader_binding_count r = 0; r < binding_count; ++r)
      {
        ac_shader_binding b;
        memcpy(&b, code, sizeof(b));
        code += sizeof(ac_shader_binding);

        ac_shader_binding ds;
        AC_ZERO(ds);
        ds.reg = b.reg;
        ds.space = space;
        ds.type = b.type;
        ds.descriptor_count = b.descriptor_count;

        if (out)
        {
          out[(*out_count)] = ds;
        }
        (*out_count)++;
      }
    }
  }

  return ac_result_success;
}

#if defined(__cplusplus)
}
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
