/*
 * (de)serialization of channels
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

#ifndef SERIALIZER_H_
#define SERIALIZER_H_

/* single channel record */
struct ChannelRec
{
  int64_t limits[LimitsNumber]; /* limits or counters (snapshot) */
  int64_t size;
  uint32_t type;

  /*
   * in fact the "name" field is not necessary since all names are ASCIIZ
   * strings and it is possible to parse names one by one using '\0' as the
   * end of string sign. however it is useful to have normal string pointers
   * in user manifest. and for snapshot this field can be set to 0
   */
  uint32_t name;
};

/* all channels */
struct ChannelsSerial
{
  uint64_t size;
  struct ChannelRec channels[0];
};

/* serialize channels into structure. all integers represented in platform order */
struct ChannelsSerial *ChannelsSerializer(
    const struct Manifest *manifest, uintptr_t offset);

/*
 * deserialize channels to manifest. channels count will be taken from
 * manifest all integers represented in platform order. return 0 if
 * successful, or negative error code
 * TODO(d'b): "channels" sanity check needed
 */
int ChannelsDeserializer(struct Manifest *manifest, struct ChannelRec *channels);

#endif /* SERIALIZER_H_ */
