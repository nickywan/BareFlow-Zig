const std = @import("std");

pub fn build(b: *std.Build) void {
    // Target x86_64 freestanding (bare metal)
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86_64,
        .os_tag = .freestanding,
        .abi = .none,
    });

    // Optimization mode
    const optimize = b.standardOptimizeOption(.{});

    // Create kernel executable
    const kernel = b.addExecutable(.{
        .name = "kernel",
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Kernel-specific flags
    kernel.setLinkerScript(b.path("src/linker.ld"));

    // Add kernel flags: code model, no red zone, no PIE
    kernel.root_module.addCMacro("__KERNEL__", "1");

    // No red zone in kernel space
    kernel.addCSourceFile(.{
        .file = b.path("src/boot.S"),
        .flags = &.{"-mno-red-zone", "-mcmodel=kernel", "-fno-pie"},
    });

    // Install the kernel
    b.installArtifact(kernel);

    // Create a run step for QEMU
    const run_cmd = b.addSystemCommand(&.{
        "qemu-system-x86_64",
        "-M", "q35",
        "-m", "128",
        "-serial", "stdio",
        "-kernel",
    });
    run_cmd.addArtifactArg(kernel);

    const run_step = b.step("run", "Run kernel in QEMU");
    run_step.dependOn(&run_cmd.step);

    // Create a test step
    const test_step = b.step("test", "Run kernel tests");
    const kernel_tests = b.addTest(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    test_step.dependOn(&b.addRunArtifact(kernel_tests).step);
}