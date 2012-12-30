/*
 * accounting.h
 *
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
