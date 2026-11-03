#include "../../third_party/eden_duo/acnh/acnh_bag.h"
#include <cassert>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <tuple>
using namespace acnh::bag;
int main(){
 for(int count:{20,30,40}){
  std::vector<uint8_t> image(ItemBag+Span);uint32_t first=20,second=count-20;
  std::memcpy(image.data()+ItemPocket+HolderCount,&first,4);std::memcpy(image.data()+ItemBag+HolderCount,&second,4);
  std::fill(image.begin()+ItemPocket+HolderFav,image.begin()+ItemPocket+HolderFav+20,NoFav);
  std::fill(image.begin()+ItemBag+HolderFav,image.begin()+ItemBag+HolderFav+20,NoFav);
  std::array<SortKey,40> keys{};std::vector<Item8> original;
  for(int i=0;i<count;i++){
   Item8 it=EmptyItem;if(i%4){it={uint8_t(1+(count-i)%7),0,0,0,uint8_t(i),0,0,0};}
   uint32_t base=i<20 ? ItemPocket : ItemBag;std::memcpy(image.data()+base+(i%20)*8,it.data(),8);
   if(i==17)image[base+HolderFav+i%20]=2;
   keys[i]={uint32_t((count-i)%3),uint32_t((count-i)%5),ItemId(it)};original.push_back(it);
  }
  std::memcpy(image.data()+ToolItem,original[17].data(),8);image[ToolLink]=0;image[ToolLink+1]=17;
  Pockets p;assert(p.FromPersonal(image.data(),image.size()));auto plan=BuildSortPlan(p,keys);assert(plan.size()<unsigned(count));
  for(auto [a,b]:plan){std::vector<Op> ops;std::string why;if(!BuildMove(p,a,b,ops,why)){std::cerr<<"move "<<a<<" -> "<<b<<" count "<<count<<" failed: "<<why<<"\n";return 1;}Pockets expected=p;assert(ApplyToPockets(ops,expected));assert(ApplyToImage(ops,image));assert(p.FromPersonal(image.data(),image.size()));assert(p.raw==expected.raw && p.link==expected.link && p.tool==expected.tool);}
  std::vector<Item8> result;bool empty=false;int favs=0;
  std::vector<std::tuple<uint32_t,uint32_t,uint16_t>> order;
  for(int i=0;i<count;i++){Slot sl;assert(SlotOf(p,i,sl));auto item=p.ItemAt(sl.holder,sl.index);if(IsEmpty(item))empty=true;else{assert(!empty);auto key=keys[item[4]];order.emplace_back(key.category,key.kind,key.id);}result.push_back(item);if(p.FavAt(sl.holder,sl.index)==2){++favs;assert(item==original[17]);}}
  assert(std::is_sorted(order.begin(),order.end()));
  std::sort(original.begin(),original.end());std::sort(result.begin(),result.end());assert(original==result && favs==1);
  Slot held;assert(SlotOf(p,LinkedSlot(p),held));assert(p.ItemAt(held.holder,held.index)==p.tool);
 }
 // Native save journals: empty insertion, existing/full fallback and malformed pools.
 auto put64=[](uint8_t *p,uint64_t x){std::memcpy(p,&x,8);};
 auto get64=[](const uint8_t *p){uint64_t x;std::memcpy(&x,p,8);return x;};
 std::array<uint8_t,48> metadata{};std::array<uint8_t,32> node{};
 constexpr uint64_t journal=0x10000,record=0x20000,total=0x3ce60;
 put64(metadata.data(),journal);put64(metadata.data()+8,journal);put64(metadata.data()+0x18,record);
 uint32_t capacity=2;std::memcpy(metadata.data()+0x28,&capacity,4);put64(node.data(),record+32);
 std::vector<JournalWrite> changes;assert(BuildDirtyJournal(journal,total,metadata,node,changes) && changes.size()==2);
 assert(changes[0].address==journal && changes[1].address==record);
 assert(get64(changes[0].value.data())==record+16 && get64(changes[0].value.data()+8)==record+16);
 assert(get64(changes[0].value.data()+0x18)==record+32 && changes[0].value[0x10]==1);
 assert(get64(changes[1].value.data())==0 && get64(changes[1].value.data()+8)==total);
 assert(get64(changes[1].value.data()+16)==journal && get64(changes[1].value.data()+24)==journal);
 std::copy_n(changes[0].value.begin(),48,metadata.begin());std::copy_n(changes[1].value.begin(),32,node.begin());
 put64(node.data(),0x120);put64(node.data()+8,8);changes.clear();
 assert(BuildDirtyJournal(journal,total,metadata,node,changes));
 assert(get64(changes[1].value.data())==0 && get64(changes[1].value.data()+8)==total);
 assert(std::equal(metadata.begin(),metadata.end(),changes[0].value.begin()));
 metadata[0x10]=2;changes.clear();assert(BuildDirtyJournal(journal,total,metadata,node,changes));
 metadata[0x10]=3;changes.clear();assert(!BuildDirtyJournal(journal,total,metadata,node,changes) && changes.empty());
 metadata[0x10]=1;put64(node.data()+16,0);assert(!BuildDirtyJournal(journal,total,metadata,node,changes));
 std::cout<<"companion sort: 20/30/40 slots, item multiset, favorites, held-tool links and native save journals passed\n";
}
