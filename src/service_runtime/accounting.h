/*
 * accounting.h
 *
 *  Created on: Oct 14, 2012
 *      Author: d'b
 */

#ifndef ACCOUNTING_H_
#define ACCOUNTING_H_

#define CGROUPS_FOLDER "/cgroups/zerovm"
#define CGROUPS_TASKS "tasks"
#define CGROUPS_USER_CPU "cpuacct.usage"
#define CGROUPS_MEMORY "memory.max_usage_in_bytes"
#define CGROUPS_SWAP "memory.memsw.max_usage_in_bytes"

/* initialize accounting */
void AccountingCtor(struct NaClApp *nap);

/* finalize accounting. return string with statistics */
void AccountingDtor(struct NaClApp *nap);

#endif /* ACCOUNTING_H_ */
