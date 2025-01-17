// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <glad/glad.h>
#include "common/common_funcs.h"
#include "common/logging/log.h"
#include "video_core/renderer_opengl/gl_state.h"
#include "video_core/renderer_opengl/gl_vars.h"

namespace OpenGL {

OpenGLState OpenGLState::cur_state;

OpenGLState::OpenGLState() {
    // These all match default OpenGL values
    cull.enabled = true;
    cull.mode = GL_BACK;
    cull.front_face = GL_CCW;

    depth.test_enabled = false;
    depth.test_func = GL_LESS;
    depth.write_mask = GL_TRUE;

    color_mask.red_enabled = GL_TRUE;
    color_mask.green_enabled = GL_TRUE;
    color_mask.blue_enabled = GL_TRUE;
    color_mask.alpha_enabled = GL_TRUE;

    stencil.test_enabled = false;
    stencil.test_func = GL_ALWAYS;
    stencil.test_ref = 0;
    stencil.test_mask = 0xFF;
    stencil.write_mask = 0xFF;
    stencil.action_depth_fail = GL_KEEP;
    stencil.action_depth_pass = GL_KEEP;
    stencil.action_stencil_fail = GL_KEEP;

    blend.enabled = false;
    blend.rgb_equation = GL_FUNC_ADD;
    blend.a_equation = GL_FUNC_ADD;
    blend.src_rgb_func = GL_ONE;
    blend.dst_rgb_func = GL_ZERO;
    blend.src_a_func = GL_ONE;
    blend.dst_a_func = GL_ZERO;
    blend.color.red = 0.0f;
    blend.color.green = 0.0f;
    blend.color.blue = 0.0f;
    blend.color.alpha = 0.0f;

    logic_op = GL_COPY;

    for (auto& texture_unit : texture_units) {
        texture_unit.texture_2d = 0;
        texture_unit.sampler = 0;
    }

    texture_cube_unit.texture_cube = 0;
    texture_cube_unit.sampler = 0;

    texture_buffer_lut_rg.texture_buffer = 0;
    texture_buffer_lut_rgba.texture_buffer = 0;

    image_shadow_buffer = 0;
    image_shadow_texture_px = 0;
    image_shadow_texture_nx = 0;
    image_shadow_texture_py = 0;
    image_shadow_texture_ny = 0;
    image_shadow_texture_pz = 0;
    image_shadow_texture_nz = 0;

    draw.read_framebuffer = 0;
    draw.draw_framebuffer = 0;
    draw.vertex_array = 0;
    draw.vertex_buffer = 0;
    draw.uniform_buffer = 0;
    draw.shader_program = 0;
    draw.program_pipeline = 0;

    scissor.enabled = false;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = 0;
    scissor.height = 0;

    viewport.x = 0;
    viewport.y = 0;
    viewport.width = 0;
    viewport.height = 0;

    clip_distance = {};

    renderbuffer = 0;
}

void OpenGLState::Apply() const {
    // Culling
    if (cull.enabled) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    glCullFace(cull.mode);

    glFrontFace(cull.front_face);

    // Depth test
    if (depth.test_enabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthFunc(depth.test_func);

    // Depth mask
    glDepthMask(depth.write_mask);

    // Color mask
    glColorMask(color_mask.red_enabled, color_mask.green_enabled, color_mask.blue_enabled,
                color_mask.alpha_enabled);

    // Stencil test
    if (stencil.test_enabled) {
        glEnable(GL_STENCIL_TEST);
    } else {
        glDisable(GL_STENCIL_TEST);
    }

    glStencilFunc(stencil.test_func, stencil.test_ref, stencil.test_mask);

    glStencilOp(stencil.action_stencil_fail, stencil.action_depth_fail, stencil.action_depth_pass);

    // Stencil mask
    glStencilMask(stencil.write_mask);

    // Blending
    if (blend.enabled != cur_state.blend.enabled) {
        if (blend.enabled) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }

        // GLES does not support glLogicOp
        if (!GLES) {
            if (blend.enabled) {
                glDisable(GL_COLOR_LOGIC_OP);
            } else {
                glEnable(GL_COLOR_LOGIC_OP);
            }
        }
    }

    glBlendColor(blend.color.red, blend.color.green, blend.color.blue, blend.color.alpha);

    glBlendFuncSeparate(blend.src_rgb_func, blend.dst_rgb_func, blend.src_a_func, blend.dst_a_func);

    glBlendEquationSeparate(blend.rgb_equation, blend.a_equation);

    // GLES does not support glLogicOp
    if (!GLES) {
        if (logic_op != cur_state.logic_op) {
            glLogicOp(logic_op);
        }
    }

    // Textures
    for (unsigned i = 0; i < ARRAY_SIZE(texture_units); ++i) {
        if (texture_units[i].texture_2d != cur_state.texture_units[i].texture_2d) {
            glActiveTexture(TextureUnits::PicaTexture(i).Enum());
            glBindTexture(GL_TEXTURE_2D, texture_units[i].texture_2d);
        }
        if (texture_units[i].sampler != cur_state.texture_units[i].sampler) {
            glBindSampler(i, texture_units[i].sampler);
        }
    }

    if (texture_cube_unit.texture_cube != cur_state.texture_cube_unit.texture_cube) {
        glActiveTexture(TextureUnits::TextureCube.Enum());
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture_cube_unit.texture_cube);
    }
    if (texture_cube_unit.sampler != cur_state.texture_cube_unit.sampler) {
        glBindSampler(TextureUnits::TextureCube.id, texture_cube_unit.sampler);
    }

    // Texture buffer LUTs
    if (texture_buffer_lut_lf.texture_buffer != cur_state.texture_buffer_lut_lf.texture_buffer) {
        glActiveTexture(TextureUnits::TextureBufferLUT_LF.Enum());
        glBindTexture(GL_TEXTURE_BUFFER, texture_buffer_lut_lf.texture_buffer);
    }

    // Texture buffer LUTs
    if (texture_buffer_lut_rg.texture_buffer != cur_state.texture_buffer_lut_rg.texture_buffer) {
        glActiveTexture(TextureUnits::TextureBufferLUT_RG.Enum());
        glBindTexture(GL_TEXTURE_BUFFER, texture_buffer_lut_rg.texture_buffer);
    }

    // Texture buffer LUTs
    if (texture_buffer_lut_rgba.texture_buffer !=
        cur_state.texture_buffer_lut_rgba.texture_buffer) {
        glActiveTexture(TextureUnits::TextureBufferLUT_RGBA.Enum());
        glBindTexture(GL_TEXTURE_BUFFER, texture_buffer_lut_rgba.texture_buffer);
    }

    // Shadow Images
    if (AllowShadow) {
        if (image_shadow_buffer != cur_state.image_shadow_buffer) {
            glBindImageTexture(ImageUnits::ShadowBuffer, image_shadow_buffer, 0, GL_FALSE, 0,
                               GL_READ_WRITE, GL_R32UI);
        }

        if (image_shadow_texture_px != cur_state.image_shadow_texture_px) {
            glBindImageTexture(ImageUnits::ShadowTexturePX, image_shadow_texture_px, 0, GL_FALSE, 0,
                               GL_READ_ONLY, GL_R32UI);
        }

        if (image_shadow_texture_nx != cur_state.image_shadow_texture_nx) {
            glBindImageTexture(ImageUnits::ShadowTextureNX, image_shadow_texture_nx, 0, GL_FALSE, 0,
                               GL_READ_ONLY, GL_R32UI);
        }

        if (image_shadow_texture_py != cur_state.image_shadow_texture_py) {
            glBindImageTexture(ImageUnits::ShadowTexturePY, image_shadow_texture_py, 0, GL_FALSE, 0,
                               GL_READ_ONLY, GL_R32UI);
        }

        if (image_shadow_texture_ny != cur_state.image_shadow_texture_ny) {
            glBindImageTexture(ImageUnits::ShadowTextureNY, image_shadow_texture_ny, 0, GL_FALSE, 0,
                               GL_READ_ONLY, GL_R32UI);
        }

        if (image_shadow_texture_pz != cur_state.image_shadow_texture_pz) {
            glBindImageTexture(ImageUnits::ShadowTexturePZ, image_shadow_texture_pz, 0, GL_FALSE, 0,
                               GL_READ_ONLY, GL_R32UI);
        }

        if (image_shadow_texture_nz != cur_state.image_shadow_texture_nz) {
            glBindImageTexture(ImageUnits::ShadowTextureNZ, image_shadow_texture_nz, 0, GL_FALSE, 0,
                               GL_READ_ONLY, GL_R32UI);
        }
    }

    // Framebuffer
    if (draw.read_framebuffer != cur_state.draw.read_framebuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, draw.read_framebuffer);
    }
    if (draw.draw_framebuffer != cur_state.draw.draw_framebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw.draw_framebuffer);
    }

    // Vertex array
    if (draw.vertex_array != cur_state.draw.vertex_array) {
        glBindVertexArray(draw.vertex_array);
    }

    // Vertex buffer
    if (draw.vertex_buffer != cur_state.draw.vertex_buffer) {
        glBindBuffer(GL_ARRAY_BUFFER, draw.vertex_buffer);
    }

    // Uniform buffer
    if (draw.uniform_buffer != cur_state.draw.uniform_buffer) {
        glBindBuffer(GL_UNIFORM_BUFFER, draw.uniform_buffer);
    }

    // Shader program
    if (draw.shader_program != cur_state.draw.shader_program) {
        glUseProgram(draw.shader_program);
    }

    // Program pipeline
    if (draw.program_pipeline != cur_state.draw.program_pipeline) {
        glBindProgramPipeline(draw.program_pipeline);
    }

    // Scissor test
    if (scissor.enabled) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    glScissor(scissor.x, scissor.y, scissor.width, scissor.height);

    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);

    // Clip distance
    if (!GLES || GLAD_GL_EXT_clip_cull_distance) {
        for (size_t i = 0; i < clip_distance.size(); ++i) {
            if (clip_distance[i] != cur_state.clip_distance[i]) {
                if (clip_distance[i]) {
                    glEnable(GL_CLIP_DISTANCE0 + static_cast<GLenum>(i));
                } else {
                    glDisable(GL_CLIP_DISTANCE0 + static_cast<GLenum>(i));
                }
            }
        }
    }

    cur_state = *this;
}

void OpenGLState::SubApply() const {
    // Depth mask
    glDepthMask(depth.write_mask);

    // Stencil mask
    glStencilMask(stencil.write_mask);

    // Color mask
    glColorMask(color_mask.red_enabled, color_mask.green_enabled, color_mask.blue_enabled,
                color_mask.alpha_enabled);

    // Framebuffer
    if (draw.read_framebuffer != cur_state.draw.read_framebuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, draw.read_framebuffer);
    }
    if (draw.draw_framebuffer != cur_state.draw.draw_framebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw.draw_framebuffer);
    }

    // Scissor test
    if (scissor.enabled) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    glScissor(scissor.x, scissor.y, scissor.width, scissor.height);

    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);

    cur_state = *this;
}

GLuint OpenGLState::BindVertexArray(GLuint array) {
    GLuint previous = cur_state.draw.vertex_array;
    glBindVertexArray(array);
    cur_state.draw.vertex_array = array;
    return previous;
}

GLuint OpenGLState::BindUniformBuffer(GLuint buffer) {
    GLuint previous = cur_state.draw.uniform_buffer;
    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    cur_state.draw.uniform_buffer = buffer;
    return previous;
}

GLuint OpenGLState::BindTexture2D(int index, GLuint texture) {
    GLuint previous = cur_state.texture_units[index].texture_2d;
    glActiveTexture(TextureUnits::PicaTexture(index).Enum());
    glBindTexture(GL_TEXTURE_2D, texture);
    cur_state.texture_units[index].texture_2d = texture;
    return previous;
}

GLuint OpenGLState::BindSampler(int index, GLuint sampler) {
    GLuint previous = cur_state.texture_units[index].sampler;
    glBindSampler(index, sampler);
    cur_state.texture_units[index].sampler = sampler;
    return previous;
}

GLuint OpenGLState::BindTextureCube(GLuint texture_cube) {
    GLuint previous = cur_state.texture_cube_unit.texture_cube;
    glActiveTexture(TextureUnits::TextureCube.Enum());
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_cube);
    cur_state.texture_cube_unit.texture_cube = texture_cube;
    return previous;
}

GLuint OpenGLState::BindReadFramebuffer(GLuint framebuffer) {
    GLuint previous = cur_state.draw.read_framebuffer;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    cur_state.draw.read_framebuffer = framebuffer;
    return previous;
}

GLuint OpenGLState::BindDrawFramebuffer(GLuint framebuffer) {
    GLuint previous = cur_state.draw.draw_framebuffer;
    if (previous != framebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        cur_state.draw.draw_framebuffer = framebuffer;
    }
    return previous;
}

GLuint OpenGLState::BindShaderProgram(GLuint program) {
    GLuint previous = cur_state.draw.shader_program;
    glUseProgram(program);
    cur_state.draw.shader_program = program;
    return previous;
}

void OpenGLState::ResetTexture(GLuint handle) {
    for (unsigned i = 0; i < ARRAY_SIZE(cur_state.texture_units); ++i) {
        if (cur_state.texture_units[i].texture_2d == handle) {
            cur_state.texture_units[i].texture_2d = 0;
            glActiveTexture(TextureUnits::PicaTexture(i).Enum());
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    if (cur_state.texture_cube_unit.texture_cube == handle) {
        cur_state.texture_cube_unit.texture_cube = 0;
        glActiveTexture(TextureUnits::TextureCube.Enum());
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    if (AllowShadow) {
        if (cur_state.image_shadow_buffer == handle) {
            cur_state.image_shadow_buffer = 0;
            glBindImageTexture(ImageUnits::ShadowBuffer, 0, 0, GL_FALSE, 0, GL_READ_WRITE,
                               GL_R32UI);
        }
        if (cur_state.image_shadow_texture_px == handle) {
            cur_state.image_shadow_texture_px = 0;
            glBindImageTexture(ImageUnits::ShadowTexturePX, 0, 0, GL_FALSE, 0, GL_READ_ONLY,
                               GL_R32UI);
        }
        if (cur_state.image_shadow_texture_nx == handle) {
            cur_state.image_shadow_texture_nx = 0;
            glBindImageTexture(ImageUnits::ShadowTextureNX, 0, 0, GL_FALSE, 0, GL_READ_ONLY,
                               GL_R32UI);
        }
        if (cur_state.image_shadow_texture_py == handle) {
            cur_state.image_shadow_texture_py = 0;
            glBindImageTexture(ImageUnits::ShadowTexturePY, 0, 0, GL_FALSE, 0, GL_READ_ONLY,
                               GL_R32UI);
        }
        if (cur_state.image_shadow_texture_ny == handle) {
            cur_state.image_shadow_texture_ny = 0;
            glBindImageTexture(ImageUnits::ShadowTextureNY, 0, 0, GL_FALSE, 0, GL_READ_ONLY,
                               GL_R32UI);
        }
        if (cur_state.image_shadow_texture_pz == handle) {
            cur_state.image_shadow_texture_pz = 0;
            glBindImageTexture(ImageUnits::ShadowTexturePZ, 0, 0, GL_FALSE, 0, GL_READ_ONLY,
                               GL_R32UI);
        }
        if (cur_state.image_shadow_texture_nz == handle) {
            cur_state.image_shadow_texture_nz = 0;
            glBindImageTexture(ImageUnits::ShadowTextureNZ, 0, 0, GL_FALSE, 0, GL_READ_ONLY,
                               GL_R32UI);
        }
    }
}

void OpenGLState::ResetSampler(GLuint handle) {
    for (unsigned i = 0; i < ARRAY_SIZE(cur_state.texture_units); ++i) {
        if (cur_state.texture_units[i].sampler == handle) {
            cur_state.texture_units[i].sampler = 0;
            glBindSampler(i, 0);
        }
    }
    if (cur_state.texture_cube_unit.sampler == handle) {
        cur_state.texture_cube_unit.sampler = 0;
        glBindSampler(TextureUnits::TextureCube.id, 0);
    }
}

void OpenGLState::ResetProgram(GLuint handle) {
    if (cur_state.draw.shader_program == handle) {
        cur_state.draw.shader_program = 0;
        glUseProgram(0);
    }
}

void OpenGLState::ResetPipeline(GLuint handle) {
    if (cur_state.draw.program_pipeline == handle) {
        cur_state.draw.program_pipeline = 0;
        glBindProgramPipeline(0);
    }
}

void OpenGLState::ResetBuffer(GLuint handle) {
    if (cur_state.draw.vertex_buffer == handle) {
        cur_state.draw.vertex_buffer = 0;
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    if (cur_state.draw.uniform_buffer == handle) {
        cur_state.draw.uniform_buffer = 0;
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void OpenGLState::ResetVertexArray(GLuint handle) {
    if (cur_state.draw.vertex_array == handle) {
        cur_state.draw.vertex_array = 0;
        glBindVertexArray(0);
    }
}

void OpenGLState::ResetFramebuffer(GLuint handle) {
    if (cur_state.draw.read_framebuffer == handle) {
        cur_state.draw.read_framebuffer = 0;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
    if (cur_state.draw.draw_framebuffer == handle) {
        cur_state.draw.draw_framebuffer = 0;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
}

} // namespace OpenGL
