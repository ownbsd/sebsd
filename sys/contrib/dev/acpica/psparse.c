/******************************************************************************
 *
 * Module Name: psparse - Parser top level AML parse routines
 *              $Revision: 146 $
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2005, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/


/*
 * Parse the AML and build an operation tree as most interpreters,
 * like Perl, do.  Parsing is done by hand rather than with a YACC
 * generated parser to tightly constrain stack and dynamic memory
 * usage.  At the same time, parsing is kept flexible and the code
 * fairly compact by parsing based on a list of AML opcode
 * templates in AmlOpInfo[]
 */

#include <contrib/dev/acpica/acpi.h>
#include <contrib/dev/acpica/acparser.h>
#include <contrib/dev/acpica/acdispat.h>
#include <contrib/dev/acpica/amlcode.h>
#include <contrib/dev/acpica/acnamesp.h>
#include <contrib/dev/acpica/acinterp.h>

#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("psparse")


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetOpcodeSize
 *
 * PARAMETERS:  Opcode          - An AML opcode
 *
 * RETURN:      Size of the opcode, in bytes (1 or 2)
 *
 * DESCRIPTION: Get the size of the current opcode.
 *
 ******************************************************************************/

UINT32
AcpiPsGetOpcodeSize (
    UINT32                  Opcode)
{

    /* Extended (2-byte) opcode if > 255 */

    if (Opcode > 0x00FF)
    {
        return (2);
    }

    /* Otherwise, just a single byte opcode */

    return (1);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsPeekOpcode
 *
 * PARAMETERS:  ParserState         - A parser state object
 *
 * RETURN:      Next AML opcode
 *
 * DESCRIPTION: Get next AML opcode (without incrementing AML pointer)
 *
 ******************************************************************************/

UINT16
AcpiPsPeekOpcode (
    ACPI_PARSE_STATE        *ParserState)
{
    UINT8                   *Aml;
    UINT16                  Opcode;


    Aml = ParserState->Aml;
    Opcode = (UINT16) ACPI_GET8 (Aml);

    if (Opcode == AML_EXTENDED_OP_PREFIX)
    {
        /* Extended opcode, get the second opcode byte */

        Aml++;
        Opcode = (UINT16) ((Opcode << 8) | ACPI_GET8 (Aml));
    }

    return (Opcode);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsCompleteThisOp
 *
 * PARAMETERS:  WalkState       - Current State
 *              Op              - Op to complete
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform any cleanup at the completion of an Op.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsCompleteThisOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_PARSE_OBJECT       *Prev;
    ACPI_PARSE_OBJECT       *Next;
    const ACPI_OPCODE_INFO  *ParentInfo;
    ACPI_PARSE_OBJECT       *ReplacementOp = NULL;


    ACPI_FUNCTION_TRACE_PTR ("PsCompleteThisOp", Op);


    /* Check for null Op, can happen if AML code is corrupt */

    if (!Op)
    {
        return_ACPI_STATUS (AE_OK);  /* OK for now */
    }

    /* Delete this op and the subtree below it if asked to */

    if (((WalkState->ParseFlags & ACPI_PARSE_TREE_MASK) != ACPI_PARSE_DELETE_TREE) ||
         (WalkState->OpInfo->Class == AML_CLASS_ARGUMENT))
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Make sure that we only delete this subtree */

    if (Op->Common.Parent)
    {
        Prev = Op->Common.Parent->Common.Value.Arg;
        if (!Prev)
        {
            /* Nothing more to do */

            goto Cleanup;
        }

        /*
         * Check if we need to replace the operator and its subtree
         * with a return value op (placeholder op)
         */
        ParentInfo = AcpiPsGetOpcodeInfo (Op->Common.Parent->Common.AmlOpcode);

        switch (ParentInfo->Class)
        {
        case AML_CLASS_CONTROL:
            break;

        case AML_CLASS_CREATE:

            /*
             * These opcodes contain TermArg operands.  The current
             * op must be replaced by a placeholder return op
             */
            ReplacementOp = AcpiPsAllocOp (AML_INT_RETURN_VALUE_OP);
            if (!ReplacementOp)
            {
                goto AllocateError;
            }
            break;

        case AML_CLASS_NAMED_OBJECT:

            /*
             * These opcodes contain TermArg operands.  The current
             * op must be replaced by a placeholder return op
             */
            if ((Op->Common.Parent->Common.AmlOpcode == AML_REGION_OP)       ||
                (Op->Common.Parent->Common.AmlOpcode == AML_DATA_REGION_OP)  ||
                (Op->Common.Parent->Common.AmlOpcode == AML_BUFFER_OP)       ||
                (Op->Common.Parent->Common.AmlOpcode == AML_PACKAGE_OP)      ||
                (Op->Common.Parent->Common.AmlOpcode == AML_VAR_PACKAGE_OP))
            {
                ReplacementOp = AcpiPsAllocOp (AML_INT_RETURN_VALUE_OP);
                if (!ReplacementOp)
                {
                    goto AllocateError;
                }
            }
            else if ((Op->Common.Parent->Common.AmlOpcode == AML_NAME_OP) &&
                     (WalkState->PassNumber <= ACPI_IMODE_LOAD_PASS2))
            {
                if ((Op->Common.AmlOpcode == AML_BUFFER_OP) ||
                    (Op->Common.AmlOpcode == AML_PACKAGE_OP) ||
                    (Op->Common.AmlOpcode == AML_VAR_PACKAGE_OP))
                {
                    ReplacementOp = AcpiPsAllocOp (Op->Common.AmlOpcode);
                    if (!ReplacementOp)
                    {
                        goto AllocateError;
                    }

                    ReplacementOp->Named.Data = Op->Named.Data;
                    ReplacementOp->Named.Length = Op->Named.Length;
                }
            }
            break;

        default:

            ReplacementOp = AcpiPsAllocOp (AML_INT_RETURN_VALUE_OP);
            if (!ReplacementOp)
            {
                goto AllocateError;
            }
        }

        /* We must unlink this op from the parent tree */

        if (Prev == Op)
        {
            /* This op is the first in the list */

            if (ReplacementOp)
            {
                ReplacementOp->Common.Parent        = Op->Common.Parent;
                ReplacementOp->Common.Value.Arg     = NULL;
                ReplacementOp->Common.Node          = Op->Common.Node;
                Op->Common.Parent->Common.Value.Arg = ReplacementOp;
                ReplacementOp->Common.Next          = Op->Common.Next;
            }
            else
            {
                Op->Common.Parent->Common.Value.Arg = Op->Common.Next;
            }
        }

        /* Search the parent list */

        else while (Prev)
        {
            /* Traverse all siblings in the parent's argument list */

            Next = Prev->Common.Next;
            if (Next == Op)
            {
                if (ReplacementOp)
                {
                    ReplacementOp->Common.Parent    = Op->Common.Parent;
                    ReplacementOp->Common.Value.Arg = NULL;
                    ReplacementOp->Common.Node      = Op->Common.Node;
                    Prev->Common.Next               = ReplacementOp;
                    ReplacementOp->Common.Next      = Op->Common.Next;
                    Next = NULL;
                }
                else
                {
                    Prev->Common.Next = Op->Common.Next;
                    Next = NULL;
                }
            }
            Prev = Next;
        }
    }


Cleanup:

    /* Now we can actually delete the subtree rooted at Op */

    AcpiPsDeleteParseTree (Op);
    return_ACPI_STATUS (AE_OK);


AllocateError:

    /* Always delete the subtree, even on error */

    AcpiPsDeleteParseTree (Op);
    return_ACPI_STATUS (AE_NO_MEMORY);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsNextParseState
 *
 * PARAMETERS:  WalkState           - Current state
 *              Op                  - Current parse op
 *              CallbackStatus      - Status from previous operation
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Update the parser state based upon the return exception from
 *              the parser callback.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsNextParseState (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_STATUS             CallbackStatus)
{
    ACPI_PARSE_STATE        *ParserState = &WalkState->ParserState;
    ACPI_STATUS             Status = AE_CTRL_PENDING;


    ACPI_FUNCTION_TRACE_PTR ("PsNextParseState", Op);


    switch (CallbackStatus)
    {
    case AE_CTRL_TERMINATE:

        /*
         * A control method was terminated via a RETURN statement.
         * The walk of this method is complete.
         */
        ParserState->Aml = ParserState->AmlEnd;
        Status = AE_CTRL_TERMINATE;
        break;


    case AE_CTRL_BREAK:

        ParserState->Aml = WalkState->AmlLastWhile;
        WalkState->ControlState->Common.Value = FALSE;
        Status = AE_CTRL_BREAK;
        break;

    case AE_CTRL_CONTINUE:


        ParserState->Aml = WalkState->AmlLastWhile;
        Status = AE_CTRL_CONTINUE;
        break;

    case AE_CTRL_PENDING:

        ParserState->Aml = WalkState->AmlLastWhile;
        break;

#if 0
    case AE_CTRL_SKIP:

        ParserState->Aml = ParserState->Scope->ParseScope.PkgEnd;
        Status = AE_OK;
        break;
#endif

    case AE_CTRL_TRUE:

        /*
         * Predicate of an IF was true, and we are at the matching ELSE.
         * Just close out this package
         */
        ParserState->Aml = AcpiPsGetNextPackageEnd (ParserState);
        break;


    case AE_CTRL_FALSE:

        /*
         * Either an IF/WHILE Predicate was false or we encountered a BREAK
         * opcode.  In both cases, we do not execute the rest of the
         * package;  We simply close out the parent (finishing the walk of
         * this branch of the tree) and continue execution at the parent
         * level.
         */
        ParserState->Aml = ParserState->Scope->ParseScope.PkgEnd;

        /* In the case of a BREAK, just force a predicate (if any) to FALSE */

        WalkState->ControlState->Common.Value = FALSE;
        Status = AE_CTRL_END;
        break;


    case AE_CTRL_TRANSFER:

        /* A method call (invocation) -- transfer control */

        Status = AE_CTRL_TRANSFER;
        WalkState->PrevOp = Op;
        WalkState->MethodCallOp = Op;
        WalkState->MethodCallNode = (Op->Common.Value.Arg)->Common.Node;

        /* Will return value (if any) be used by the caller? */

        WalkState->ReturnUsed = AcpiDsIsResultUsed (Op, WalkState);
        break;


    default:

        Status = CallbackStatus;
        if ((CallbackStatus & AE_CODE_MASK) == AE_CODE_CONTROL)
        {
            Status = AE_OK;
        }
        break;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsParseAml
 *
 * PARAMETERS:  WalkState       - Current state
 *
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse raw AML and return a tree of ops
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsParseAml (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;
    ACPI_THREAD_STATE       *Thread;
    ACPI_THREAD_STATE       *PrevWalkList = AcpiGbl_CurrentWalkList;
    ACPI_WALK_STATE         *PreviousWalkState;


    ACPI_FUNCTION_TRACE ("PsParseAml");

    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
        "Entered with WalkState=%p Aml=%p size=%X\n",
        WalkState, WalkState->ParserState.Aml,
        WalkState->ParserState.AmlSize));


    /* Create and initialize a new thread state */

    Thread = AcpiUtCreateThreadState ();
    if (!Thread)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    WalkState->Thread = Thread;
    AcpiDsPushWalkState (WalkState, Thread);

    /*
     * This global allows the AML debugger to get a handle to the currently
     * executing control method.
     */
    AcpiGbl_CurrentWalkList = Thread;

    /*
     * Execute the walk loop as long as there is a valid Walk State.  This
     * handles nested control method invocations without recursion.
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "State=%p\n", WalkState));

    Status = AE_OK;
    while (WalkState)
    {
        if (ACPI_SUCCESS (Status))
        {
            /*
             * The ParseLoop executes AML until the method terminates
             * or calls another method.
             */
            Status = AcpiPsParseLoop (WalkState);
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
            "Completed one call to walk loop, %s State=%p\n",
            AcpiFormatException (Status), WalkState));

        if (Status == AE_CTRL_TRANSFER)
        {
            /*
             * A method call was detected.
             * Transfer control to the called control method
             */
            Status = AcpiDsCallControlMethod (Thread, WalkState, NULL);

            /*
             * If the transfer to the new method method call worked, a new walk
             * state was created -- get it
             */
            WalkState = AcpiDsGetCurrentWalkState (Thread);
            continue;
        }
        else if (Status == AE_CTRL_TERMINATE)
        {
            Status = AE_OK;
        }
        else if ((Status != AE_OK) && (WalkState->MethodDesc))
        {
            ACPI_REPORT_METHOD_ERROR ("Method execution failed",
                WalkState->MethodNode, NULL, Status);

            /* Ensure proper cleanup */

            WalkState->ParseFlags |= ACPI_PARSE_EXECUTE;

            /* Check for possible multi-thread reentrancy problem */

            if ((Status == AE_ALREADY_EXISTS) &&
                (!WalkState->MethodDesc->Method.Semaphore))
            {
                /*
                 * This method is marked NotSerialized, but it tried to create
                 * a named object, causing the second thread entrance to fail.
                 * We will workaround this by marking the method permanently
                 * as Serialized.
                 */
                WalkState->MethodDesc->Method.MethodFlags |= AML_METHOD_SERIALIZED;
                WalkState->MethodDesc->Method.Concurrency = 1;
            }
        }

        /* We are done with this walk, move on to the parent if any */

        WalkState = AcpiDsPopWalkState (Thread);

        /* Reset the current scope to the beginning of scope stack */

        AcpiDsScopeStackClear (WalkState);

        /*
         * If we just returned from the execution of a control method,
         * there's lots of cleanup to do
         */
        if ((WalkState->ParseFlags & ACPI_PARSE_MODE_MASK) == ACPI_PARSE_EXECUTE)
        {
            if (WalkState->MethodDesc)
            {
                /* Decrement the thread count on the method parse tree */

                WalkState->MethodDesc->Method.ThreadCount--;
            }

            AcpiDsTerminateControlMethod (WalkState);
        }

        /* Delete this walk state and all linked control states */

        AcpiPsCleanupScope (&WalkState->ParserState);

        PreviousWalkState = WalkState;

        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
            "ReturnValue=%p, ImplicitValue=%p State=%p\n",
            WalkState->ReturnDesc, WalkState->ImplicitReturnObj, WalkState));

        /* Check if we have restarted a preempted walk */

        WalkState = AcpiDsGetCurrentWalkState (Thread);
        if (WalkState)
        {
            if (ACPI_SUCCESS (Status))
            {
                /*
                 * There is another walk state, restart it.
                 * If the method return value is not used by the parent,
                 * The object is deleted
                 */
                if (!PreviousWalkState->ReturnDesc)
                {
                    Status = AcpiDsRestartControlMethod (WalkState,
                                PreviousWalkState->ImplicitReturnObj);
                }
                else
                {
                    /*
                     * We have a valid return value, delete any implicit
                     * return value.
                     */
                    AcpiDsClearImplicitReturn (PreviousWalkState);

                    Status = AcpiDsRestartControlMethod (WalkState,
                                PreviousWalkState->ReturnDesc);
                }
                if (ACPI_SUCCESS (Status))
                {
                    WalkState->WalkType |= ACPI_WALK_METHOD_RESTART;
                }
            }
            else
            {
                /* On error, delete any return object */

                AcpiUtRemoveReference (PreviousWalkState->ReturnDesc);
            }
        }

        /*
         * Just completed a 1st-level method, save the final internal return
         * value (if any)
         */
        else if (PreviousWalkState->CallerReturnDesc)
        {
            if (PreviousWalkState->ImplicitReturnObj)
            {
                *(PreviousWalkState->CallerReturnDesc) =
                    PreviousWalkState->ImplicitReturnObj;
            }
            else
            {
                 /* NULL if no return value */

                *(PreviousWalkState->CallerReturnDesc) =
                    PreviousWalkState->ReturnDesc;
            }
        }
        else
        {
            if (PreviousWalkState->ReturnDesc)
            {
                /* Caller doesn't want it, must delete it */

                AcpiUtRemoveReference (PreviousWalkState->ReturnDesc);
            }
            if (PreviousWalkState->ImplicitReturnObj)
            {
                /* Caller doesn't want it, must delete it */

                AcpiUtRemoveReference (PreviousWalkState->ImplicitReturnObj);
            }
        }

        AcpiDsDeleteWalkState (PreviousWalkState);
    }

    /* Normal exit */

    AcpiExReleaseAllMutexes (Thread);
    AcpiUtDeleteGenericState (ACPI_CAST_PTR (ACPI_GENERIC_STATE, Thread));
    AcpiGbl_CurrentWalkList = PrevWalkList;
    return_ACPI_STATUS (Status);
}

