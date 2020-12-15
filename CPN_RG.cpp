//
// Created by hecong on 2020/11/26.
//
#include "CPN_RG.h"

/*check the completeness of transition tt:
* whether all related variables of @parm tt have been assigned a value;
* if not, return the first unassigned variable's vid from @parm vid
* */
bool binding::completeness(vector<VARID> &unassignedvar) const {
    bool complete = true;
    const CPN_Transition &tt = cpn->transition[tranid];
    vector<VARID>::const_iterator iter;
    for(iter=tt.relvararray.begin();iter!=tt.relvararray.end();++iter) {
        if(vararray[*iter] == MAXCOLORID) {
            complete = false;
            unassignedvar.push_back(*iter);
        }
    }
    return complete;
}

void binding::printBinding(string &str) {
    str = "<";
    for(int i=0;i<cpn->varcount-1;++i) {
        if(vararray[i]!=MAXCOLORID) {
            str += VarTable::vartable[i].id + "=" + to_string(vararray[i])+",";
        }
        else {
            str += VarTable::vartable[i].id + "=" + "-,";
        }
    }
    if(vararray[cpn->varcount-1]!=MAXCOLORID) {
        str += VarTable::vartable[cpn->varcount-1].id + "=" +to_string(vararray[cpn->varcount-1]);
    }
    else {
        str += VarTable::vartable[cpn->varcount-1].id + "=" + "-";
    }
    str += ">";
}

bool binding::operator>(const binding &otherbind) {
    if(this->tranid > otherbind.tranid)
        return true;
    else if(this->tranid < otherbind.tranid)
        return false;

    /*this->tranid = otherbind.tranid*/
    /*compare the vararry*/
    return array_greater(this->vararray,otherbind.vararray,cpn->varcount);
}

binding::binding(SHORTIDX tid) {
    tranid = tid;
    next = NULL;
    vararray = new COLORID [cpn->varcount];
    for(int i=0;i<cpn->varcount;++i) {
        vararray[i] = MAXCOLORID;
    }
//    memset(vararray,MAXCOLORID,sizeof(COLORID)*cpn->varcount);
}

binding::binding(const binding &b) {
    tranid = b.tranid;
    next = NULL;
    vararray = new COLORID [cpn->varcount];
    memcpy(this->vararray,b.vararray,sizeof(COLORID)*(cpn->varcount));
}

binding::~binding() {
    delete [] vararray;
}

BindingList::BindingList() {
    listhead = new binding(0);
    strategy = byhead;
}

BindingList::~BindingList() {
    binding *p = listhead,*q;
    while (p) {
        q=p->next;
        delete p;
        p=q;
    }
}

void BindingList::insert(binding *p) {
    if(strategy == byorder) {
        /*strategy1:insert by order*/
        binding *present,*predecessor;
        present = listhead->next;
        predecessor = listhead;
        while (present) {
            if(*p > *present) {
                p->next = present;
                predecessor->next = p;
                return;
            }
            predecessor = present;
            present = present->next;
        }
        predecessor->next = p;
        p->next = present;
    }
    else if(strategy == byhead) {
        /*strategy2:insert by head*/
        p->next = listhead->next;
        listhead->next = p;
    }
}

void BindingList::copy_insert(SHORTIDX tranid,const COLORID *vararry) {
    binding *p = new binding(tranid);
    memcpy(p->vararray,vararry,sizeof(COLORID)*cpn->varcount);

    if(strategy == byorder) {
        /*strategy1:insert by order*/
        binding *present,*predecessor;
        present = listhead->next;
        predecessor = listhead;
        while (present) {
            if(*p > *present) {
                p->next = present;
                predecessor->next = p;
                return;
            }
            predecessor = present;
            present = present->next;
        }
        predecessor->next = p;
        p->next = present;
    }
    else if(strategy == byhead) {
        p->next = listhead->next;
        listhead->next = p;
    }
}

binding *BindingList::Delete(binding *outelem) {
    binding *p,*q;
    p=listhead->next;
    q=listhead;
    while (p) {
        if(p==outelem) {
            q->next = p->next;
            delete p;
            return q->next;
        }
        q=p;
        p=p->next;
    }
}

bool BindingList::empty() {
    if(listhead->next==NULL)
        return true;
    else
        return false;
}

CPN_RGNode::CPN_RGNode() {
    marking = new Multiset[cpn->placecount];
    markingid = 0;
    for(int i=0;i<cpn->placecount;++i) {
        marking[i].initiate(cpn->place[i].tid,cpn->place[i].sid);
    }
    next = NULL;
    Binding_Available = false;
}

CPN_RGNode::~CPN_RGNode() {
    delete [] marking;
//    MallocExtension::instance()->ReleaseFreeMemory();
}

index_t CPN_RGNode::Hash(SHORTNUM *weight) {
    index_t Hashvalue = 0;
    for(int i=0;i<cpn->placecount;++i) {
        if(marking[i].hashvalue == 0)
            marking[i].Hash();
        Hashvalue += weight[i] * marking[i].hashvalue;
    }
    return Hashvalue;
}

void CPN_RGNode::printMarking() {
    cout<<"[M"<<markingid<<"]"<<endl;
    for(int i=0;i<cpn->placecount;++i)
    {
        cout<<setiosflags(ios::left)<<setw(15)<<cpn->place[i].id<<":";
        this->marking[i].printToken();
    }
    /*print fireable bindings*/
    binding *p = enbindings.listhead->next;
    while (p) {
        cout<<"{";
        cout<<cpn->transition[p->tranid].id<<",";
        string bind;
        p->printBinding(bind);
        cout<<bind<<"}"<<endl;
        p=p->next;
    }
    cout<<"----------------------------------------"<<endl;
}

bool CPN_RGNode::operator==(const CPN_RGNode &state) {
    bool equal = true;
    for(int i=0;i<cpn->placecount;++i) {
        //检查库所i的tokenmetacount是否一样
        SHORTNUM tmc = this->marking[i].colorcount;
        if(tmc != state.marking[i].colorcount)
        {
            equal = false;
            break;
        }

        //检查每一个tokenmeta是否一样
        Tokens *t1=this->marking[i].tokenQ->next;
        Tokens *t2=state.marking[i].tokenQ->next;
        for(t1,t2; t1!=NULL && t2!=NULL; t1=t1->next,t2=t2->next)
        {
            //先检查tokencount
            if(t1->tokencount!=t2->tokencount)
            {
                equal = false;
                break;
            }

            //检查color
            type tid = cpn->place[i].tid;
            if(tid == dot) {
                continue;
            }
            else if(tid == usersort || tid == finiteintrange){
                COLORID cid1,cid2;
                t1->tokenvalue->getColor(cid1);
                t2->tokenvalue->getColor(cid2);
                if(cid1!=cid2) {
                    equal = false;
                    break;
                }
            }
            else if(tid == productsort){
                COLORID *cid1,*cid2;
                SORTID sid = cpn->place[i].sid;
                int sortnum = SortTable::productsort[sid].sortnum;
                cid1 = new COLORID[sortnum];
                cid2 = new COLORID[sortnum];
                t1->tokenvalue->getColor(cid1,sortnum);
                t2->tokenvalue->getColor(cid2,sortnum);
                for(int k=0;k<sortnum;++k)
                {
                    if(cid1[k]!=cid2[k])
                    {
                        equal = false;
                        delete [] cid1;
                        delete [] cid2;
                        break;
                    }
                }
                if(equal==false)
                    break;
                delete [] cid1;
                delete [] cid2;
            }
        }
        if(equal == false)
            break;
    }
    return equal;
}

void CPN_RGNode::operator=(const CPN_RGNode &state) {
    int i;
    for(i=0;i<cpn->placecount;++i)
    {
        const Multiset &placemark = state.marking[i];
        MultisetCopy(this->marking[i],placemark,placemark.tid,placemark.sid);
    }
}

void CPN_RGNode::all_FireableBindings() {
    if(Binding_Available == true)
        return;

    for(SHORTIDX i=0;i<cpn->transitioncount;++i) {
        tran_FireableBindings(i);
    }
    Binding_Available = true;
}

void CPN_RGNode::tran_FireableBindings(SHORTIDX tranid) {
    CPN_Transition &tran = cpn->transition[tranid];
    BindingList *bindingList = new BindingList[tran.producer.size()];

    BindingList initlist;
    binding *p = new binding(tranid);
    initlist.insert(p);

    if(tran.producer[0].arc_exp.get_IBS(marking[tran.producer[0].idx],initlist,bindingList[0])!=SUCCESS) {
        delete [] bindingList;
        return;
    }

    for(int i=1;i<tran.producer.size();++i) {
        if(tran.producer[i].arc_exp.get_IBS(marking[tran.producer[i].idx],bindingList[i-1],bindingList[i])!=SUCCESS) {
            delete [] bindingList;
            return;
        }
    }

    binding *q = bindingList[tran.producer.size()-1].listhead->next;
    while (q) {
        bool complete;
        vector<VARID> unassignedvar;
        complete = q->completeness(unassignedvar);
        if(complete) {
            enbindings.copy_insert(tranid,q->vararray);
        }
        else {
            this->complete(unassignedvar,0,q);
        }
        q=q->next;
    }

    q = enbindings.listhead->next;
    bool guardtruth = true;
    while (q) {
        if(q->tranid != tranid) {
            q=q->next;
            continue;
        }
        //judge guard
        if(tran.hasguard) {
            guardtruth=tran.guard.judgeGuard(q->vararray);
        }

        if(!guardtruth) {
            q = enbindings.Delete(q);
        }
        else {
            q=q->next;
        }
    }
    delete [] bindingList;
}

void CPN_RGNode::complete(const vector<VARID> unassignedvar,int level,binding *inbind) {
    if(level>=unassignedvar.size()) {
        enbindings.copy_insert(inbind->tranid,inbind->vararray);
        return;
    }

    const Variable &curvar = VarTable::vartable[unassignedvar[level]];
    if(curvar.tid == usersort) {
        SHORTNUM feconstnum = SortTable::usersort[curvar.sid].feconstnum;
        for(int i=0;i<feconstnum;++i) {
            inbind->vararray[curvar.vid] = i;
            complete(unassignedvar,level+1,inbind);
        }
    }
    else if(curvar.tid == finiteintrange) {
        int start = SortTable::finitintrange[curvar.sid].start;
        int end = SortTable::finitintrange[curvar.sid].end;
        for(int i=start;i<=end;++i) {
            inbind->vararray[curvar.vid] = i;
            complete(unassignedvar,level+1,inbind);
        }
    }
}

bool CPN_RGNode::isfirable(string transname) {
    bool fireable = false;
    map<string,index_t>::iterator finditer;
    finditer = cpn->mapTransition.find(transname);
    if(finditer!=cpn->mapTransition.end()) {
        while(!Binding_Available) {
            all_FireableBindings();
        }
        binding *p = enbindings.listhead->next;
        while (p) {
            if(p->tranid == finditer->second) {
                fireable = true;
                break;
            }
            p=p->next;
        }
        return fireable;
    }
    else {
        consistency = false;
//        cerr<<"[ERROR @ CPN_RGNode::isfirable] can not find transition \""<<transname<<"\"."<<endl;
//        exit(-1);
    }
}

CPN_RG::CPN_RG() {
    initnode = NULL;
    markingtable = new CPN_RGNode*[CPNRGTABLE_SIZE];
    for(int i=0;i<CPNRGTABLE_SIZE;++i) {
        markingtable[i] = NULL;
    }
    nodecount = 0;
    hash_conflict_times = 0;

    weight = new SHORTNUM[cpn->placecount];
    srand((int)time(NULL));
    for(int j=0;j<cpn->placecount;++j)
    {
        weight[j] = random(133)+1;
    }
}

CPN_RG::~CPN_RG() {
    for(int i=0;i<CPNRGTABLE_SIZE;++i)
    {
        if(markingtable[i]!=NULL)
        {
            CPN_RGNode *p=markingtable[i];
            CPN_RGNode *q;
            while(p)
            {
                q=p->next;
                delete p;
                p=q;
            }
        }
    }
    delete [] markingtable;
//    MallocExtension::instance()->ReleaseFreeMemory();
}

void CPN_RG::addRGNode(CPN_RGNode *state) {
    state->markingid = nodecount;
    index_t hashvalue = state->Hash(weight);
    index_t size = CPNRGTABLE_SIZE-1;
    hashvalue = hashvalue & size;

    if(markingtable[hashvalue]!=NULL) {
        hash_conflict_times++;
        //cerr<<"<"<<mark->numid<<","<<markingtable[hashvalue]->numid<<">"<<endl;
    }

    state->next = markingtable[hashvalue];
    markingtable[hashvalue] = state;
    nodecount++;
}

CPN_RGNode *CPN_RG::CPNRGinitnode() {
    initnode = new CPN_RGNode;
    //遍历每一个库所
    for(int i=0;i<cpn->placecount;++i)
    {
        CPlace &pp = cpn->place[i];
        MultisetCopy(initnode->marking[i],pp.initM,pp.tid,pp.sid);
        initnode->marking[i].Hash();
    }

//    initnode->all_FireableBindings();
//    initnode->printMarking();

    addRGNode(initnode);
    return initnode;
}

bool CPN_RG::NodeExist(CPN_RGNode *state, CPN_RGNode *&existstate) {
    index_t hashvalue = state->Hash(weight);
    index_t size = CPNRGTABLE_SIZE-1;
    hashvalue = hashvalue & size;

    bool exist = false;
    CPN_RGNode *p = markingtable[hashvalue];
    while(p)
    {
        if(*p == *state)
        {
            exist = true;
            existstate = p;
            break;
        }
        p=p->next;
    }
    return exist;
}

void CPN_RG::Generate() {
    if(initnode == NULL)
        CPNRGinitnode();
    Generate(initnode);
}

void CPN_RG::Generate(CPN_RGNode *state) {
    binding *p = state->enbindings.listhead->next;
    bool exist = false;
    while (p) {
        CPN_RGNode *child = RGNode_Child(state,p,exist);
        if(!exist) {
            Generate(child);
        }
        p=p->next;
    }
}

CPN_RGNode *CPN_RG::RGNode_Child(CPN_RGNode *curstate, binding *bind, bool &exist) {
    CPN_RGNode *child = new CPN_RGNode;
    *child = *curstate;

    CPN_Transition &tran = cpn->transition[bind->tranid];
    vector<CSArc>::iterator front;
    for(front=tran.producer.begin();front!=tran.producer.end();++front) {
        Multiset arcms;
        front->arc_exp.to_Multiset(arcms,bind->vararray);
        MINUS(child->marking[front->idx],arcms);
    }

    vector<CSArc>::iterator rear;
    for(rear=tran.consumer.begin();rear!=tran.consumer.end();++rear) {
        Multiset arcms;
        rear->arc_exp.to_Multiset(arcms,bind->vararray);
        PLUS(child->marking[rear->idx],arcms);
    }

    for(int i=0;i<cpn->placecount;++i) {
        child->marking[i].Hash();
    }

    CPN_RGNode *existnode =NULL;
    exist = NodeExist(child,existnode);
    if(exist) {
//        cout<<"----------------------------------"<<endl;
//        cout<<"M"<<curstate->markingid<<"[>"<<tran.id<<" M"<<existnode->markingid<<endl;
//        cout<<"----------------------------------"<<endl;
        delete child;
        return existnode;
    } else {
//        child->all_FireableBindings();
        addRGNode(child);
//        cout<<"M"<<curstate->markingid<<"[>"<<tran.id;
//        child->printMarking();
//        cout<<"***NODECOUNT:"<<nodecount<<"***"<<endl;
        return child;
    }
}