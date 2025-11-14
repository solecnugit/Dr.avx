
/*
 * Code Manipulation API Sample:
 * avx512count.cpp
 *
 * Reports the dynamic count of AVX-512 instructions executed and their ratio
 * to all instructions executed. Results are written to "instr_stat.txt".
 * Illustrates how to identify and count specific instruction types.
 * Based on the inscount.cpp sample from DynamoRIO.
 *
 * The runtime options for this client include:
 *   -only_from_app  Do not count instructions in shared libraries.
 *   -detailed       Show detailed counts for different AVX-512 instruction types.
 *   -outfile        Specify an output file (default: instr_stat.txt).
 * The options are handled using the droption extension.
 */

// #include "dr_api.h"
// #include "dr_events.h"
// #include "drmgr.h"
// #include "droption.h"
// #include <string.h>
// #include <unordered_map>
// #include <string>

// #define PREFIX_EVEX 0x000100000

// namespace dynamorio {
// namespace samples {
// namespace {

// using ::dynamorio::droption::droption_parser_t;
// using ::dynamorio::droption::DROPTION_SCOPE_ALL;
// using ::dynamorio::droption::DROPTION_SCOPE_CLIENT;
// using ::dynamorio::droption::droption_t;

// #ifdef WINDOWS
// #    define DISPLAY_STRING(msg) dr_messagebox(msg)
// #else
// #    define DISPLAY_STRING(msg) dr_printf("%s\n", msg);
// #endif

// #define NULL_TERMINATE(buf) (buf)[(sizeof((buf)) / sizeof((buf)[0])) - 1] = '\0'

// static droption_t<bool> only_from_app(
//     DROPTION_SCOPE_CLIENT, "only_from_app", false,
//     "Only count app, not lib, instructions",
//     "Count only instructions in the application itself, ignoring instructions in "
//     "shared libraries.");

// static droption_t<bool>
//     detailed(DROPTION_SCOPE_CLIENT, "detailed", false,
//              "Show detailed counts for different AVX-512 instruction types",
//              "Show detailed counts for different categories of AVX-512 instructions.");

// static droption_t<std::string> outfile(
//     DROPTION_SCOPE_CLIENT, "outfile", "instr_stat.txt",
//     "Output file for instruction statistics",
//     "Specify the output file for instruction statistics (default: instr_stat.txt).");

// /* Application module */
// static app_pc exe_start;

// /* Counters for instructions */
// static uint64_t avx512_count = 0;
// static uint64_t total_count = 0;

// /* File for output */
// static file_t output_file;

// /* A simple clean call that will be automatically inlined */
// static void
// avx512_counter(uint64 num_avx512, uint64 num_total)
// {
//     avx512_count += num_avx512;
//     total_count += num_total;
// }

// static void
// print_instruction_info(instr_t *instr)
// {
//     char disasm[256];
//     void *drcontext = dr_get_current_drcontext();

//     /* Disassemble the instruction */
//     instr_disassemble_to_buffer(drcontext, instr, disasm, sizeof(disasm));

//     /* Get the address */
//     app_pc pc = instr_get_app_pc(instr);

//     /* Print the instruction with its address */
//     dr_fprintf(1, "AVX-512 instruction at %p: %s\n", pc, disasm);
// }

// static void
// event_exit(void);
// // static dr_emit_flags_t
// // event_bb_analysis(void *drcontext, void *tag, instrlist_t *bb, bool for_trace,
// //                   bool translating, void **user_data);

// static bool
// is_evex(instr_t *instr)
// {
//     opnd_t src0 = instr_get_src(instr, 0);
//     return opnd_is_reg(src0) && reg_is_opmask(opnd_get_reg(src0));
// }
// /* Check if an instruction is an AVX-512 instruction */
// static bool
// is_avx512_instruction(instr_t *instr)
// {

//     if (is_evex(instr) ||
//         (instr_get_opcode(instr) >= OP_kmovw && instr_get_opcode(instr) <= OP_ktestd)) {

//         return true;
//     }

//     return false;
// }

// static void
// event_exit(void)
// {
//     char msg[1024];
//     int len;
//     double ratio = 0.0;

//     if (total_count > 0) {
//         ratio = ((double)avx512_count / (double)total_count) * 100.0;
//     }

//     len = dr_snprintf(msg, sizeof(msg) / sizeof(msg[0]),
//                       "AVX-512 Instruction Analysis Results👇\n"
//                       "  Total instructions executed: %llu\n"
//                       "  AVX-512 instructions executed: %llu\n"
//                       "  AVX-512 instruction ratio: %.4f%%\n",
//                       total_count, avx512_count, ratio);
//     DR_ASSERT(len > 0);
//     NULL_TERMINATE(msg);

//     /* Write to output file */
//     dr_fprintf(output_file, "%s\n", msg);

//     /* Close the output file */
//     dr_close_file(output_file);

//     drmgr_exit();
// }
// 
// static dr_emit_flags_t
// event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr,
//                       bool for_trace, bool translating, void *user_data)
// {
//     uint64 num_avx512instrs = 0;
//     uint64 num_total = 0;

//     drmgr_disable_auto_predication(drcontext, bb);

//     if (!drmgr_is_first_instr(drcontext, instr))
//         return DR_EMIT_DEFAULT;

//     /* Count instructions */
//     for (instr = instrlist_first(bb); instr != NULL; instr = instr_get_next(instr)) {
//         if (!instr_is_app(instr))
//             continue;

//         num_total++;

//         if (is_avx512_instruction(instr)) {
//             num_avx512instrs++;
//             print_instruction_info(instr);
//         }
//     }

//     dr_insert_clean_call(drcontext, bb, instrlist_first_app(bb), (void *)avx512_counter,
//                          false, 2, OPND_CREATE_INT64(num_avx512instrs),
//                          OPND_CREATE_INT64(num_total));

//     return DR_EMIT_DEFAULT;
// }

// static dr_emit_flags_t
// event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr,
//                       bool for_trace, bool translating, void *user_data)
// {
//     uint64 num_avx512instrs = 0;
//     uint64 num_total = 0;

//     drmgr_disable_auto_predication(drcontext, bb);

//     if (!drmgr_is_first_instr(drcontext, instr))
//         return DR_EMIT_DEFAULT;

//     /* Count instructions in this block */
//     for (instr_t *app_instr = instrlist_first(bb); app_instr != NULL; 
//          app_instr = instr_get_next(app_instr)) {
//         if (!instr_is_app(app_instr))
//             continue;

//         num_total++;

//         if (is_avx512_instruction(app_instr)) {
//             num_avx512instrs++;
//             if (detailed.get_value())
//                 print_instruction_info(app_instr);
//         }
//     }

//     /* Insert a clean call that will execute each time this block executes */
//     if (num_total > 0) {
//         dr_insert_clean_call(drcontext, bb, instrlist_first_app(bb), 
//                              (void *)avx512_counter, false, 2, 
//                              OPND_CREATE_INT64(num_avx512instrs),
//                              OPND_CREATE_INT64(num_total));
//     }

//     return DR_EMIT_DEFAULT;
// }

// } // namespace
// } // namespace samples
// } // namespace dynamorio

// DR_EXPORT void /* per block counting */
// dr_client_main(client_id_t id, int argc, const char *argv[])
// {
//     dr_set_client_name("DynamoRIO Sample Client 'avx512count'",
//                        "http://dynamorio.org/issues");

//     /* Parse options */
//     if (!dynamorio::droption::droption_parser_t::parse_argv(
//             dynamorio::droption::DROPTION_SCOPE_CLIENT, argc, argv, NULL, NULL))
//         DR_ASSERT(false);
//     drmgr_init();

//     /* Get main module address */
//     if (dynamorio::samples::only_from_app.get_value()) {
//         module_data_t *exe = dr_get_main_module();
//         if (exe != NULL)
//             dynamorio::samples::exe_start = exe->start;
//         dr_free_module_data(exe);
//     }

//     /* Open output file */
//     dynamorio::samples::output_file = dr_open_file(
//         dynamorio::samples::outfile.get_value().c_str(), DR_FILE_WRITE_OVERWRITE);
//     DR_ASSERT(dynamorio::samples::output_file != INVALID_FILE);

//     /* Register events */
//     dr_register_exit_event(dynamorio::samples::event_exit);
//     drmgr_register_bb_instrumentation_event(
//         NULL,
//         dynamorio::samples::event_app_instruction,
//         NULL);

//     /* Log initialization */
//     dr_log(NULL, DR_LOG_ALL, 1, "Client 'avx512count' initializing\n");
//     dr_fprintf(STDERR, "Client avx512count is running, will output to %s\n",
//                dynamorio::samples::outfile.get_value().c_str());
// }

/* -------------------------------------------------------------------------- */

/*
 * Code Manipulation API Sample:
 * avx512count.cpp
 *
 * Reports the dynamic count of AVX-512 instructions executed and their ratio
 * to all instructions executed. Results are written to "instr_stat.txt".
 * Illustrates how to identify and count specific instruction types.
 * Based on the inscount.cpp sample from DynamoRIO.
 *
 * The runtime options for this client include:
 *   -detailed       Show detailed counts for different AVX-512 instruction types.
 *   -outfile        Specify an output file (default: instr_stat.txt).
 * The options are handled using the droption extension.
 */

#include "dr_api.h"
#include "dr_events.h"
#include "drmgr.h"
#include "droption.h"
#include "drx.h"
#include <string.h>
#include <string>

namespace dynamorio {
namespace samples {
namespace {

using ::dynamorio::droption::droption_parser_t;
using ::dynamorio::droption::DROPTION_SCOPE_ALL;
using ::dynamorio::droption::DROPTION_SCOPE_CLIENT;
using ::dynamorio::droption::droption_t;

#ifdef WINDOWS
#    define DISPLAY_STRING(msg) dr_messagebox(msg)
#else
#    define DISPLAY_STRING(msg) dr_printf("%s\n", msg);
#endif

#define NULL_TERMINATE(buf) (buf)[(sizeof((buf)) / sizeof((buf)[0])) - 1] = '\0'

static droption_t<bool>
    detailed(DROPTION_SCOPE_CLIENT, "detailed", false,
             "Show detailed counts for different AVX-512 instruction types",
             "Show detailed counts for different categories of AVX-512 instructions.");

static droption_t<std::string> outfile(
    DROPTION_SCOPE_CLIENT, "outfile", "instr_stat.txt",
    "Output file for instruction statistics",
    "Specify the output file for instruction statistics (default: instr_stat.txt).");

/* Counters for instructions */
static uint64_t avx512_count = 0;
static uint64_t total_count = 0;

/* File for output */
static file_t output_file;

/* Check if an instruction uses EVEX prefix (which most AVX-512 instructions use) */
static bool
is_evex(instr_t *instr)
{
    opnd_t src0 = instr_get_src(instr, 0);
    return opnd_is_reg(src0) && reg_is_opmask(opnd_get_reg(src0));
}

/* Check if an instruction is an AVX-512 instruction */
static bool
is_avx512_instruction(instr_t *instr)
{
    if (is_evex(instr) ||
        (instr_get_opcode(instr) >= OP_kmovw && instr_get_opcode(instr) <= OP_ktestd)) {
        return true;
    }
    return false;
}

/* Print information about an AVX-512 instruction */
static void
print_instruction_info(instr_t *instr)
{
    char disasm[256];
    void *drcontext = dr_get_current_drcontext();

    /* Disassemble the instruction */
    instr_disassemble_to_buffer(drcontext, instr, disasm, sizeof(disasm));

    /* Get the address */
    app_pc pc = instr_get_app_pc(instr);

    /* Print the instruction with its address */
    dr_fprintf(output_file, "AVX-512 instruction at %p: %s\n", pc, disasm);
}

/* Exit event - print final statistics */
static void
event_exit(void)
{
    char msg[1024];
    int len;
    double ratio = 0.0;

    if (total_count > 0) {
        ratio = ((double)avx512_count / (double)total_count) * 100.0;
    }

    len = dr_snprintf(msg, sizeof(msg) / sizeof(msg[0]),
                      "AVX-512 Instruction Analysis Results👇\n"
                      "  Total instructions executed: %llu\n"
                      "  AVX-512 instructions executed: %llu\n"
                      "  AVX-512 instruction ratio: %.4f%%\n",
                      total_count, avx512_count, ratio);
    DR_ASSERT(len > 0);
    NULL_TERMINATE(msg);

    /* Write to output file */
    dr_fprintf(output_file, "%s\n", msg);
    DISPLAY_STRING(msg);

    /* Close the output file */
    dr_close_file(output_file);

    /* Clean up */
    drx_exit();
    drmgr_exit();
}

/* Instrument each basic block to count instructions dynamically */
static dr_emit_flags_t
event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr,
                      bool for_trace, bool translating, void *user_data)
{
    /* Skip if not the first instruction or if translating */
    if (!drmgr_is_first_instr(drcontext, instr) || translating)
        return DR_EMIT_DEFAULT;

    /* We need to disable auto-predication for proper counting on ARM */
    drmgr_disable_auto_predication(drcontext, bb);

    /* Insert per-instruction counter updates */
    for (instr_t *app_instr = instrlist_first(bb); app_instr != NULL; 
         app_instr = instr_get_next(app_instr)) {
        if (!instr_is_app(app_instr))
            continue;
            
        /* Increment total count for each app instruction */
        drx_insert_counter_update(drcontext, bb, app_instr, SPILL_SLOT_1,
                                  &total_count, 1, DRX_COUNTER_LOCK);
        
        /* Check if it's an AVX-512 instruction */
        if (is_avx512_instruction(app_instr)) {
            /* Print details if requested */
            if (detailed.get_value())
                print_instruction_info(app_instr);
                
            /* Increment AVX-512 count */
            drx_insert_counter_update(drcontext, bb, app_instr, SPILL_SLOT_1,
                                      &avx512_count, 1, DRX_COUNTER_LOCK);
        }
    }
    
    return DR_EMIT_DEFAULT;
}

} // namespace
} // namespace samples
} // namespace dynamorio

/* DynamoRIO client entry point */
DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{
    dr_set_client_name("DynamoRIO Sample Client 'avx512count'",
                       "http://dynamorio.org/issues");

    /* Parse options */
    if (!dynamorio::droption::droption_parser_t::parse_argv(
            dynamorio::droption::DROPTION_SCOPE_CLIENT, argc, argv, NULL, NULL))
        DR_ASSERT(false);
            
    /* Initialize extensions */
    drmgr_init();
    drx_init();
    
    /* Open output file */
    dynamorio::samples::output_file = dr_open_file(
        dynamorio::samples::outfile.get_value().c_str(), DR_FILE_WRITE_OVERWRITE);
    DR_ASSERT(dynamorio::samples::output_file != INVALID_FILE);
    
    /* Register events */
    dr_register_exit_event(dynamorio::samples::event_exit);
    if (!drmgr_register_bb_instrumentation_event(NULL,
                                              dynamorio::samples::event_app_instruction,
                                              NULL)) {
        DR_ASSERT(false); /* Cannot continue */
    }
    
    /* Log initialization */
    dr_log(NULL, DR_LOG_ALL, 1, "Client 'avx512count' initializing\n");
    dr_fprintf(STDERR, "Client avx512count is running, will output to %s\n",
               dynamorio::samples::outfile.get_value().c_str());
}
