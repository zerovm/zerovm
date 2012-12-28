/*
 * accounting.h
 *
 *  Created on: Oct 14, 2012
 *      Author: d'b
 */

#ifndef ACCOUNTING_H_
#define ACCOUNTING_H_

#define DEFAULT_ACCOUNTING "not available"
#define STAT_FMT "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu"

/* initialize accounting */
void AccountingCtor(struct NaClApp *nap);

/* finalize accounting. return string with statistics */
void AccountingDtor(struct NaClApp *nap);

/* return accounting string */
const char *GetAccountingInfo();

#endif /* ACCOUNTING_H_ */
