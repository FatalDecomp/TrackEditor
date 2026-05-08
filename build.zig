// Zig build for ROLLER Track Editor.
//
// Phase 1 (current): WhipLib static library compiles on macOS, Linux, Windows.
// Phase 2: ModelExporter / TrackAnalyzer / CarPlansParser CLI tools (need FBX SDK).
// Phase 3: TrackEditor Qt5 GUI (needs moc/uic/rcc orchestration).

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Build-time toggles. FBX SDK ships precompiled binaries that are not in
    // the repo; until they're installed under external/FBX/lib/, the FBX
    // exporter cannot compile.
    const enable_fbx = b.option(
        bool,
        "fbx",
        "Compile FBXExporter.cpp (requires FBX SDK installed at external/FBX/)",
    ) orelse false;

    const whiplib = buildWhipLib(b, target, optimize, enable_fbx);
    b.installArtifact(whiplib);
}

fn buildWhipLib(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    enable_fbx: bool,
) *std.Build.Step.Compile {
    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    mod.addIncludePath(b.path("WhipLib"));
    mod.addIncludePath(b.path("WhipLib/CarPlans"));
    mod.addIncludePath(b.path("WhipLib/SignPlans"));
    mod.addIncludePath(b.path("external/glm"));
    mod.addIncludePath(b.path("external/stb"));
    // The codebase mixes `#include <glew.h>` and `#include "glew.h"` without a
    // `GL/` prefix, so both directories must be on the search path.
    mod.addIncludePath(b.path("external/glew/include"));
    mod.addIncludePath(b.path("external/glew/include/GL"));

    mod.addCMacro("GLM_ENABLE_EXPERIMENTAL", "1");
    // GLEW pulls in <GL/glu.h> by default. The GLU library is deprecated and
    // unused by this codebase (verified by grep), and asking for it makes
    // cross-compilation to Linux brittle (no glu headers in zig's sysroot).
    mod.addCMacro("GLEW_NO_GLU", "1");
    // WhipLib.h's WLFUNC macro defaults to __declspec(dllimport) on Windows
    // unless one of WHIPLIB_LIB / WHIPLIB_DLL is defined. We're building a
    // static archive, so flag that to keep the implementations linkable.
    mod.addCMacro("WHIPLIB_LIB", "1");

    addPlatformIncludes(mod, target);

    if (enable_fbx) {
        mod.addIncludePath(b.path("external/FBX/include"));
    }

    const cpp_flags: []const []const u8 = &.{
        "-std=c++17",
        "-fPIC",
        // Match the upstream Linux Makefile defaults.
    };

    var sources = std.ArrayList([]const u8){};
    defer sources.deinit(b.allocator);
    sources.appendSlice(b.allocator, &whiplib_core_sources) catch @panic("OOM");
    if (enable_fbx) {
        sources.append(b.allocator, "FBXExporter.cpp") catch @panic("OOM");
    }

    mod.addCSourceFiles(.{
        .root = b.path("WhipLib"),
        .files = sources.items,
        .flags = cpp_flags,
    });

    return b.addLibrary(.{
        .name = "WhipLib",
        .root_module = mod,
        .linkage = .static,
    });
}

// Hardcoded brewed prefixes are fine for now — once Phase 2 lands we'll
// resolve these via `brew --prefix` invoked from a build step or pkg-config.
fn addPlatformIncludes(mod: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    switch (target.result.os.tag) {
        .macos => {
            // GL_SILENCE_DEPRECATION mutes the wall of OpenGL warnings macOS
            // emits since 10.14. The editor uses GLEW for GL bindings so the
            // deprecated system headers aren't actually called.
            mod.addCMacro("GL_SILENCE_DEPRECATION", "1");

            // Apple Silicon Homebrew lives under /opt/homebrew; Intel under
            // /usr/local. Add both — the missing one is harmless.
            mod.addSystemIncludePath(.{ .cwd_relative = "/opt/homebrew/include" });
            mod.addSystemIncludePath(.{ .cwd_relative = "/opt/homebrew/opt/libxml2/include/libxml2" });
            mod.addSystemIncludePath(.{ .cwd_relative = "/usr/local/include" });
            mod.addSystemIncludePath(.{ .cwd_relative = "/usr/local/opt/libxml2/include/libxml2" });
        },
        .linux => {
            mod.addSystemIncludePath(.{ .cwd_relative = "/usr/include/libxml2" });
        },
        else => {},
    }
}

// Mirrors the SRC list in WhipLib/Makefile, minus FBXExporter.cpp (gated by
// -Dfbx). Keep alphabetised for diff-friendliness.
const whiplib_core_sources = [_][]const u8{
    "Camera.cpp",
    "CarHelpers.cpp",
    "Clock.cpp",
    "DisasmHelpers.cpp",
    "DriveComponent.cpp",
    "Entity.cpp",
    "GameClock.cpp",
    "GameInput.cpp",
    "IndexBuffer.cpp",
    "Logging.cpp",
    "MathHelpers.cpp",
    "NoclipComponent.cpp",
    "ObjExporter.cpp",
    "ObjImporter.cpp",
    "Palette.cpp",
    "PhysicsComponent.cpp",
    "Renderer.cpp",
    "Scene.cpp",
    "SceneManager.cpp",
    "Shader.cpp",
    "ShapeComponent.cpp",
    "ShapeData.cpp",
    "ShapeFactory.cpp",
    "Texture.cpp",
    "Track.cpp",
    "TrackComponent.cpp",
    "Unmangler.cpp",
    "VertexArray.cpp",
    "VertexBuffer.cpp",
    "WhipLib.cpp",
};

const std = @import("std");
