/**************************************************************************

Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/
#ifndef ERROR_HANDLE_H_
#define ERROR_HANDLE_H_

/** Utility macro to ease NULL error handling (return) with automatic debug print.

   @param[in]  Par       Parameter/Variable that should be checked against NULL
   @param[in]  RetVal    Value that should be returned when NULL
**/
#define IF_NULL_RETURN(Par, RetVal)                                               \
  if ((Par) == NULL) {                                                            \
    DEBUGPRINT (CRITICAL, ("error - " #Par " is NULL, returning %r\n", RetVal));  \
    return (RetVal);                                                              \
  }

/** Utility macro to ease NULL error handling (return) with automatic debug print (2 variables).

   @param[in]  Par1      Parameter/Variable that should be checked against NULL
   @param[in]  Par2      Parameter/Variable that should be checked against NULL
   @param[in]  RetVal    Value that should be returned when NULL
**/
#define IF_NULL2_RETURN(Par1, Par2, RetVal) \
        IF_NULL_RETURN (Par1, RetVal)       \
        IF_NULL_RETURN (Par2, RetVal)

/** Utility macro to ease NULL error handling (return) with automatic debug print (3 variables).

   @param[in]  Par1      Parameter/Variable that should be checked against NULL
   @param[in]  Par2      Parameter/Variable that should be checked against NULL
   @param[in]  Par3      Parameter/Variable that should be checked against NULL
   @param[in]  RetVal    Value that should be returned when NULL
**/
#define IF_NULL3_RETURN(Par1, Par2, Par3, RetVal) \
        IF_NULL2_RETURN (Par1, Par2, RetVal)      \
        IF_NULL_RETURN (Par3, RetVal)

/** Utility macro to ease NULL error handling (return) with automatic debug print (4 variables).

   @param[in]  Par1      Parameter/Variable that should be checked against NULL
   @param[in]  Par2      Parameter/Variable that should be checked against NULL
   @param[in]  Par3      Parameter/Variable that should be checked against NULL
   @param[in]  Par4      Parameter/Variable that should be checked against NULL
   @param[in]  RetVal    Value that should be returned when NULL
**/
#define IF_NULL4_RETURN(Par1, Par2, Par3, Par4, RetVal) \
        IF_NULL3_RETURN (Par1, Par2, Par3, RetVal)      \
        IF_NULL_RETURN (Par4, RetVal)

/** Utility macro to ease error handling (goto) with automatic debug print.

   @param[in]  Condition  Condition, when TRUE goto is executed
   @param[in]  GotoLabel  Label to which goto should jump
**/
#define IF_GOTO(Condition, GotoLabel)                                               \
  if (Condition) {                                                                  \
    DEBUGPRINT (CRITICAL, ("error - " #Condition ", jumping to " #GotoLabel "\n")); \
    goto GotoLabel;                                                                 \
  }

/** Utility macro to ease error handling (return) with automatic debug print.

   @param[in]  Condition  Condition, when TRUE return is executed
   @param[in]  RetVal     Value that should be returned
**/
#define IF_RETURN(Condition, RetVal)                                            \
  if (Condition) {                                                              \
    DEBUGPRINT (CRITICAL, ("error - " #Condition ", returning %d \n", RetVal)); \
    return RetVal;                                                              \
  }


/** Utility macro to ease SC error handling (return) with automatic debug print.

   @param[in]  ScStatus   Shared Code status, when != SUCCESS return is executed
   @param[in]  RetVal     Value that should be returned
**/
#define IF_SCERR_RETURN(ScStatus, RetVal)                                        \
  if ((ScStatus) != SC_SUCCESS) {                                                \
    DEBUGPRINT (CRITICAL, ("SC error - %d, returning %d \n", ScStatus, RetVal)); \
    return RetVal;                                                               \
  }

/** NULL assert utility macro (2 variables).

   @param[in]  Par1      Parameter/Variable that should be checked against NULL
   @param[in]  Par2      Parameter/Variable that should be checked against NULL
**/
#define ASSERT_IF_NULL2(Par1, Par2) \
  ASSERT ((Par1) != NULL);          \
  ASSERT ((Par2) != NULL)

/** NULL assert utility macro (3 variables).

   @param[in]  Par1      Parameter/Variable that should be checked against NULL
   @param[in]  Par2      Parameter/Variable that should be checked against NULL
   @param[in]  Par3      Parameter/Variable that should be checked against NULL
**/
#define ASSERT_IF_NULL3(Par1, Par2, Par3) \
  ASSERT_IF_NULL2 (Par1, Par2);           \
  ASSERT ((Par3) != NULL)

#endif /* ERROR_HANDLE_H_ */
