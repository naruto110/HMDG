#ifndef __GRAPH_H_
#define __GRAPH_H_

#include "global.h"

size_v findIdleBlk(vector<idle_blk*> &idle_blk_vec,size_v degree)
{
	size_v cap = degree + ceil((float)BUF_PRO * degree) + 3;  // +3 �����洢 id degree offset
	for (size_v blkIdx = 0; blkIdx < idle_blk_vec.size(); blkIdx++)
	{
		if (idle_blk_vec[blkIdx]->max_cap >= cap)
		{
			idle_blk_vec[blkIdx]->max_cap -= cap;  // ����max_cap
			idle_blk_vec[blkIdx]->next_pos = idle_blk_vec[blkIdx]->next_pos - cap + 3;
			return idle_blk_vec[blkIdx]->blk_idx;
		}
	}
	return -1;
}

void toVertexV(const WorkerParams& params, char* line, PMEMoid root)
{
	// ���ݸ�ʽ��id |degree| neighbor-0 neighbor-1 ... 
	char* pch;
	pch = strtok(line, "\t");
	size_v id = atoi(pch);  // for larger id, use atol

	if (id > global_max_id)
	{
		global_max_id = id;
	}

	pch = strtok(NULL, " ");//skip neighbor_number
	size_v degree = atoi(pch);

	//cout << "ID: " << id << " degree: " << degree << endl;

	if (degree >= MAX_DEG_CAP) // ������һ��blkװ���£�����buffer��
	{
		size_v numblk = ceil((float)degree / (MAX_LEN-3)); // ֻ�����ڽڵ�洢��������buffer

		//cout << "numblk: " << numblk << endl;

		size_v nei_mod = degree % (MAX_LEN - 3);  // ���һ��blk�洢���ڽڵ�����
		for (size_v i = 0; i < numblk; i++)
		{
			graph_blk* gblk = generate_new_blk(root);

			// �洢���� ǰ��blk�洢��ǰ���blk������buffer�����洢 MAX_LEN - 3��neighbors
			gblk->vcnt = 1;
			gblk->buf[0] = id;
			
			gblk->buf[2] = MAX_LEN - 1;  // offset ��ĩλ��ʼ

			size_v neicnt = MAX_LEN - 3; // ���ڵ������㣬����blk����ܴ��ڽڵ�����
			size_v start_idx = MAX_LEN-1;  // �Ӻ���ǰ�洢
			while ((neicnt--) && (pch = strtok(NULL, " "))) // �����ڽڵ�
			{
				size_v nb = atoi(pch);
				gblk->buf[start_idx] = nb;

				//cout << "index: " << start_idx << endl;

				start_idx--;
			}
			
			if (i < numblk - 1) // �������һ��blk������blk��next_blk����-1
			{
				gblk->buf[1] = MAX_LEN - 3;  // ��ǰblk�洢��MAX_LEN - 3���ڽڵ�,��������
				gblk->next_blk = gblk->blk_id + 1;  // ȷ��֪������Ҫ�½�һ��blk
			}
			else   // ���һ��blk
			{
				gblk->buf[1] = nei_mod;  //  ���һ��blkδ����
			}
		}
	}
	else  // һ��blk��װ�£����ȴ� idle_blk_vec ��ѡȡblk���д洢
	{
		size_v blkIdx = findIdleBlk(idle_blk_vec, degree);
		if (blkIdx != -1)  // �ҵ����ʵ�blk
		{
			graph_blk* gblk = graph_blk_vec[blkIdx];
			size_v start_idx = gblk->vcnt * 3;
			size_v preoff = gblk->buf[start_idx - 1];
			size_v predegree = gblk->buf[start_idx - 2];
			size_v prebuf = ceil((float)predegree * BUF_PRO);
			size_v offset = preoff - predegree - prebuf;  // ǰһ���������ʼoffset - degree - buffer���ȣ���Ϊ��ǰ������ʼoffset

			gblk->buf[start_idx] = id;
			gblk->buf[start_idx + 1] = degree;
			gblk->buf[start_idx + 2] = offset;

			while (pch = strtok(NULL, " ")) // �����ڽڵ�
			{
				size_v nb = atoi(pch);
				gblk->buf[offset] = nb;
				offset--;
			}
			gblk->vcnt++;

			size_v cur_cap = degree + ceil((float)degree * BUF_PRO);
			gblk->next_pos -= cur_cap;
		}
		else  // �½�һ���µ�blk
		{
			graph_blk* gblk = generate_new_blk(root);

			gblk->buf[0] = id;
			gblk->buf[1] = degree;
			size_v offset = MAX_LEN - 1;
			gblk->buf[2] = offset;  //  ��ʼ�洢λ��

			while (pch = strtok(NULL, " ")) // �����ڽڵ�
			{
				size_v nb = atoi(pch);
				gblk->buf[offset] = nb;
				offset--;
			}
			gblk->vcnt++;

			size_v cur_cap = degree + ceil((float)degree * BUF_PRO);
			gblk->next_pos -= cur_cap;

			size_v cap = ceil((float)degree * BUF_PRO) + degree + 3;
			size_v capmod = MAX_LEN - cap;

			idle_blk* iblk = (idle_blk*)malloc(sizeof(idle_blk));  // in DRAM

			iblk->blk_idx = gblk->blk_id;
			iblk->max_cap = capmod;
			idle_blk_vec.push_back(iblk);
		}
	}
}

void toEdge(const WorkerParams& params, char* line)
{
	// ���ݸ�ʽ��src_id des_id
	new_edge e;
	char* pch;
	pch = strtok(line, " ");
	size_v src = atoi(pch); 

	global_max_id = max(global_max_id, src);

	pch = strtok(NULL, " ");
	size_v des = atoi(pch);

	if (src == des)
		return;

	e.src = src; // ����������Ҳ���ɢ
	e.des = des;
	// e.src = des;  // �Ҳ��������������ɢ
	// e.des = src;

	e.statue = 1; 
	global_edge_vec.push_back(e);
	global_edge_num++;

	e.src = des;
	e.des = src;
	global_edge_vec.push_back(e);
	global_edge_num++;
}

void toEdgeMix(const WorkerParams& params, char* line)
{
	// ���ݸ�ʽ��src_id des_id
	new_edge e;
	char* pch;
	pch = strtok(line, " ");
	size_v src = atoi(pch); 

	global_max_id = max(global_max_id, src);
	

	pch = strtok(NULL, " ");
	size_v des = atoi(pch);

	global_max_id = max(global_max_id, des);

	pch = strtok(NULL, " ");
	int status = atoi(pch);

	// int status = 0;
	// cout << status << " ";

	// cout << "status: " << status << " " << pch << " ";

	if (src == des)
		return;

	e.src = src; // ����������Ҳ���ɢ
	e.des = des;
	e.statue = status;
	// e.src = des;  // �Ҳ��������������ɢ
	// e.des = src;

	// e.statue = 1; 
	global_edge_vec.push_back(e);
	global_edge_num++;

	// e.src = des;
	// e.des = src;
	// global_edge_vec.push_back(e);
	// global_edge_num++;
}

void load_graph(const WorkerParams& params, PMEMoid root)
{
	double start_t = get_current_time();
	string file_path;
	file_path = params.input_path;
	cout << " file: " << file_path << endl;

	ifstream ori_file(file_path.c_str());
	string line;
	int linecnt = 0;
	if (ori_file) // �и��ļ�  
	{
		while (getline(ori_file, line)) // line�в�����ÿ�еĻ��з�  
		{
			if (line.size() != 0)
			{
				linecnt++;
				//cout << "test: line---: " << linecnt << endl;
				toVertexV(params, const_cast<char*>(line.c_str()), root);
				// toEdge(params, const_cast<char*>(line.c_str()), root);
				// if (linecnt && linecnt % 10000000 == 0) {
				// 	cout << "load " << (linecnt / 10000000) << " edges" << endl;
				// }
			}
		}
		ori_file.close();
		//	cout << "worker: " << _my_rank << " load data over!!!" << endl;
	}
	else // û�и��ļ�  
	{
		cout << params.input_path.c_str() << endl;
		exit(-1);
	}

	double end_t = get_current_time();
	cout << "Load graph time: " << end_t - start_t << endl;
	// ��������id���Ӧλ������(����Ҫ)
	//init(vertices);
}

void load_graph_dram(const WorkerParams& params)
{
	double start_t = get_current_time();
	string file_path;
	file_path = params.input_path;
	cout << " file: " << file_path << endl;

	ifstream ori_file(file_path.c_str());
	string line;
	int64_t linecnt = 0;
	if (ori_file) // �и��ļ�  
	{
		while (getline(ori_file, line)) // line�в�����ÿ�еĻ��з�  
		{
			if (line.size() != 0)
			{
				linecnt++;  // for test
				// if (linecnt == 26500)
				// {
				// 	return;
				// }
				// cout << "test: line---: " << linecnt << endl;
				// toVertexV(params, const_cast<char*>(line.c_str()), root);
				// toEdge(params, const_cast<char*>(line.c_str()));
				toEdgeMix(params, const_cast<char*>(line.c_str()));
			
			}
		}
		ori_file.close();
		cout << "total lines: " << linecnt << endl;
		//	cout << "worker: " << _my_rank << " load data over!!!" << endl;
	}
	else // û�и��ļ�  
	{
		cout << params.input_path.c_str() << endl;
		exit(-1);
	}

	double end_t = get_current_time();
	cout << "Load graph from disk time: " << end_t - start_t << endl;
	// ��������id���Ӧλ������(����Ҫ)
	//init(vertices);
}

void print_csr(graph_blk* blk)
{
	cout << "graph print begin. Number of vertices: " << blk->vcnt << endl;
	int32_t startIdx = 0;
	int32_t offsetIdx = MAX_LEN;
	cout << "blk->buf: {";
	for (int32_t i=0; i<offsetIdx; i++)
	{
		cout << blk->buf[i] << ", ";
	}
	cout << "}" << endl;

	for (int32_t vidx=0; vidx < blk->vcnt; vidx++)
	{
		size_v vid = blk->buf[2*vidx];
		int32_t neiBeginIdx = blk->buf[2*vidx + 1];
		int32_t neiEndIdx = blk->buf[2*vidx + 3];

		// cout << "neiEndIdx: " << neiEndIdx-neiBeginIdx << endl;

		if (vidx == blk->vcnt-1)
		{
			neiEndIdx = blk->next_pos;
		}
		cout << "id: " << vid << " neiNum: " << neiBeginIdx - neiEndIdx << " neighbor list: {";
		for (int32_t neiIdx=neiBeginIdx; neiIdx > neiEndIdx; neiIdx--)
		{
			cout << blk->buf[neiIdx] << ", ";
		}
		cout << "}" << endl;
	}
	cout << "graph print complete." << endl;
}

void sorted_csr_pm(const WorkerParams& params, graph_blk* blk)  
{
	/*
		sorted���壺���ȶԱ�src��С��Ȼ��Ա�des��С
	*/
	double start_t = get_current_time();
	blk->blk_id = 0;
	blk->vcnt = 0;

	blk->next_pos = MAX_LEN - 1; // �����һλ��ʼ�洢�ڽڵ���Ϣ
	for (int edgeIdx = 0; edgeIdx < global_edge_vec.size(); edgeIdx++)
	{
		new_edge p = global_edge_vec[edgeIdx];
		size_v src = p.src;
		size_v des = p.des;

		// ��һ��������ʵ��CSR�洢
		if (blk->vcnt == 0)  // �����һ����
		{
			int32_t startIdx = 2 * blk->vcnt;
			blk->buf[startIdx] = src;
			blk->buf[startIdx+1] = blk->next_pos;
			blk->buf[blk->next_pos] = des;

			startIdx += 2; // ÿ������洢id��offset

			blk->vcnt += 1;
			blk->next_pos--; // �´δ洢�ڽڵ㣬--����Ϊ�ڽڵ�洢�Ӻ���ǰ
			continue;
		}

		// cout << "test---number of vertices: " << blk->vcnt << " id: " << blk->buf[0] << " " << blk->buf[MAX_LEN - 1] << endl;

		for (int32_t vidx = 0; vidx < blk->vcnt; vidx++)
		{
			/* ���滻Ϊ��������srcλ�� */
			int32_t vid = blk->buf[2 * vidx];
			int32_t nextvid = blk->buf[2 * vidx + 2];
			int32_t nextoff = blk->buf[2 * vidx + 3];
			if (src == vid)  
			{
				/* ���ڸ�src��ֻ�ı�neighbor lish���� */
				int32_t nextBeginIdx = blk->buf[2 * vidx + 3]; // ��һ������ڽڵ���ʼλ��
				if (vidx < (blk->vcnt-1)) // �����������һ����src
				{
					int32_t tmpOff = blk->buf[2 * vidx + 1];  // �Ѵ����һ���������ʼ�ڽڵ�λ��
					int32_t neiIdx = tmpOff;
					for (; neiIdx > nextBeginIdx; neiIdx--)
					{
						if (des < blk->buf[neiIdx])  // ����������������
						{
							//  ���к���Ԫ�غ���һλ
							for (int32_t j = blk->next_pos; j < neiIdx; j++)
							{
								blk->buf[j] = blk->buf[j+1];
							}
							blk->buf[neiIdx] = des;
							blk->next_pos--; // ��һ����λ
							break;
						}
						// ��Ҫ���뵱ǰ�����ڽӱ����һ��λ��
						if ((neiIdx == nextBeginIdx+1) && (des > blk->buf[nextBeginIdx+1]))
						{
							for (int32_t j = blk->next_pos; j < nextBeginIdx; j++)
							{
								blk->buf[j] = blk->buf[j+1];
							}
							blk->buf[nextBeginIdx] = des;
							blk->next_pos--; // ��һ����λ
						}
					}
					// ��ǰ����֮�����ж����Offset����Ҫ-1��������һλ
					for (int32_t remainvIdx = vidx+1; remainvIdx < blk->vcnt; remainvIdx++)
					{
						blk->buf[2*remainvIdx+1] -= 1;
					}
				}
				else // ��ǰsrc�������һ����ֱ���ڼ���Ԫ�ز����򼴿�. vidx = blk->vcnt-1
				{
					int32_t tmpOff = blk->buf[2 * vidx + 1];  // �Ѵ����һ���������ʼ�ڽڵ�λ��
					int32_t neiIdx = tmpOff;
					for (; neiIdx > blk->next_pos; neiIdx--)
					{
						if (des < blk->buf[neiIdx])  // ����������������
						{
							//  ���к���Ԫ�غ���һλ
							for (int32_t j = blk->next_pos; j < neiIdx; j++)
							{
								blk->buf[j] = blk->buf[j+1];
							}
							blk->buf[neiIdx] = des;
							blk->next_pos--; // ��һ����λ
							break;
						}

						if ((neiIdx == blk->next_pos+1) && (des > blk->buf[blk->next_pos+1]))
						{
							blk->buf[blk->next_pos] = des;
							blk->next_pos--; // ��һ����λ
						}
					}
				}
				break;
			}
			else if (src>vid && vidx==(blk->vcnt-1))  // ����Чnextid
			{
				/* ��ǰ�����ڸ�src������src�����ڽڵ㣬ֻ��ע��src�����򣬲���ĩβ */
				blk->buf[2*blk->vcnt] = src;
				blk->buf[2*blk->vcnt+1] = blk->next_pos;

				blk->buf[blk->next_pos] = des;
				blk->next_pos--; // ��һ����λ
				blk->vcnt += 1;
				break;
			}
			else if (src>vid && src<nextvid) // ������Чnextid
			{
				/* ��ǰ�����ڸ�src������src�����ڽڵ㣬ֻ��ע��src������, �������ж����м� */
				// cout << "test--- vidx: " << vidx << " vcnt: " << blk->vcnt << endl;
				for (int32_t j = blk->vcnt; j>vidx; j--) // �ƶ�����id��offsetλ��
				{
					blk->buf[2*j] = blk->buf[2*j-2];  // id
					blk->buf[2*j+1] = blk->buf[2*j-1]-1;  // offset����Ϊ��Ҫ����һ����ֵ����Ҫ��-1
				}

				blk->buf[2*vidx+2] = src;
				blk->buf[2*vidx+3] = nextoff;

				for (int32_t j = blk->next_pos; j<nextoff; j++)
				{
					blk->buf[j] = blk->buf[j+1];
				}
				blk->buf[nextoff] = des;
				blk->next_pos--; // ��һ����λ
				blk->vcnt += 1;

				// print_csr(blk);
				break;
			}

		}
	}

	double end_t = get_current_time();
	// print graph
	// print_csr(blk);
	cout << "Load graph in CSR: " << end_t - start_t << endl;
}


void print_edges(void)
{
	cout << "{";
	for (int i = 0; i < global_edge_vec.size(); i++)
	{
		cout << "(" << global_edge_vec[i].src << ", " << global_edge_vec[i].des << ", " << global_edge_vec[i].statue << "), ";
	}
	cout << "}" << endl;
}

void generate_vertex_index_meta(vector<graph_blk*> graphBlkVec, vector<VIDX_PAIR> &vertexIdxMata)
{
	size_v total_blk = graphBlkVec.size();
	for (size_v blkIdx = 0; blkIdx < total_blk; blkIdx++)
	{
		size_v vertex_num = graphBlkVec[blkIdx]->vcnt;
		size_v bid = graphBlkVec[blkIdx]->blk_id;
		for (size_v vIdx = 0; vIdx < vertex_num; vIdx++)
		{
			size_v vid = graphBlkVec[blkIdx]->buf[vIdx * 3];
			VIDX_PAIR tmp_pair(blkIdx, vIdx);
			vertexIdxMata[vid] = tmp_pair;
		}
	}
}

size_v detect_buffer(graph_blk* gblk, size_v v_offset)
{
	/*
		��֤һ�������buffer�Ƿ񹻵ķ�����
			1. ����offset - degree ��= next v offset
			2. ĩ�˶�����Ҫ���⿼��
	*/
	size_v contributor_offset = -1;
	size_v hop = 1;
	size_v forward_flag = 1;
	size_v backward_flag = 1;
	size_v cur_offset = v_offset;

	while (true)
	{
		if (forward_flag == 1)
		{
			cur_offset = v_offset + hop;  // ��ӵ�cur_offset���㴦��buffer

			if (cur_offset == gblk->vcnt - 1)   // ���һ�����㴦�Ƿ��п���slot
			{
			//	if (gblk->buf[cur_offset * 3 + 2] - gblk->buf[cur_offset * 3 + 1] > cur_offset * 3 + 2)
				if (gblk->buf[cur_offset * 3 + 2] - gblk->buf[cur_offset * 3 + 1] > gblk->next_pos)
				{
					return cur_offset;
				}
			}
			else
			{
				if (cur_offset >= gblk->vcnt)  // �˴�offset��0��ʼ����������vcntʱ�Ѿ�������ǰblk�洢���ܶ�����
				{
					forward_flag = -1;  // ����������
				}
				else
				{
					if (gblk->buf[cur_offset * 3 + 2] - gblk->buf[cur_offset * 3 + 1] > gblk->buf[(cur_offset + 1) * 3 + 2])  // cur_offset��buffer
					{
						return cur_offset;
					}
				}
			}
		}

		if (backward_flag == 1)
		{
			cur_offset = v_offset - hop;
			if (cur_offset < 0)
			{
				backward_flag = -1;  // ������ǰ��
			}
			else
			{
				if (gblk->buf[cur_offset * 3 + 2] - gblk->buf[cur_offset * 3 + 1] > gblk->buf[(cur_offset + 1) * 3 + 2])  // cur_offset��buffer
				{
					return cur_offset;
				}
			}
		}

		if (forward_flag == -1 && backward_flag == -1)
		{
			return -1;  // û�п���buffer slot
		}
		hop++;
	}
}

void exchange_slot(PMEMobjpool* pop, graph_blk* gblk, size_v owner, size_v curv, size_v value)
{
	/*
		curv����ǰ��Ҫ�����ڽڵ�Ķ��㣨pos��gblk�еĵڼ������㣩
		owner����curv�����ӵ�п���buffer slot�Ķ��㣨pos��
		value����Ҫ����curv���ڽڵ�id
	*/
	size_v stride; // slot���ƻ�������
	size_v curoff = gblk->buf[curv * 3 + 2];
	size_v curdeg = gblk->buf[curv * 3 + 1];


	if (owner > curv) 
	{
		stride = 1;  // owner�ڵ�ǰ�����Ҳ࣬����slot�ڵ�ǰ�������
	}
	else
	{
		stride = -1; // owner�ڵ�ǰ������࣬����slot�ڵ�ǰ�����Ҳ�
	}

	if (stride == 1) // (�ڽڵ��б������)
	{
		while (owner != curv)  // move the slot
		{
			size_v owner_degree = gblk->buf[owner * 3 + 1];
			size_v owner_off = gblk->buf[owner * 3 + 2];
			gblk->buf[owner_off - owner_degree] = gblk->buf[owner_off];
			pmemobj_persist(pop, &(gblk->buf[owner_off - owner_degree]), sizeof(size_v));
			gblk->buf[owner * 3 + 2] = owner_off - 1;
			pmemobj_persist(pop, &(gblk->buf[owner * 3 + 2]), sizeof(size_v));

			owner -= stride;
		}
	}
	else
	{
		owner = owner + 1; // ��ඥ�㣨offset���󣩴���һ�ڽڵ㿪ʼ�ƶ�slot (�ڽڵ��б����Ҳ�)
		while (owner <= curv)
		{
			size_v owner_off = gblk->buf[owner * 3 + 2];
			size_v lastNeiPos = gblk->buf[owner * 3 + 2] - gblk->buf[owner * 3 + 1] + 1;  // owner���һ���ڽڵ�洢��λ�ã�offset - degree + 1

			gblk->buf[owner_off + 1] = gblk->buf[lastNeiPos];  // ���һ���ڽڵ㸴�Ƶ��Ҳ�һ����slot��
			pmemobj_persist(pop, &(gblk->buf[owner_off + 1]), sizeof(size_v));
			gblk->buf[owner * 3 + 2] = owner_off + 1;
			pmemobj_persist(pop, &(gblk->buf[owner * 3 + 2]), sizeof(size_v));

			owner -= stride;
		}
	}

	curoff = gblk->buf[curv * 3 + 2];
	gblk->buf[curoff - curdeg] = value;
	pmemobj_persist(pop, &(gblk->buf[curoff - curdeg]), sizeof(size_v));
	gblk->buf[curv * 3 + 1] = curdeg + 1;
	pmemobj_persist(pop, &(gblk->buf[curv * 3 + 1]), sizeof(size_v));
}

void update_vertex_index(vector<VIDX_PAIR>& vertexIdxMata, graph_blk* target_blk, size_v vid)
{
	size_v vertex_offset = target_blk->vcnt - 1;
	vertexIdxMata[vid].first = target_blk->blk_id;
	vertexIdxMata[vid].second = vertex_offset;
}

void remalloc_vertex(PMEMobjpool* pop, PMEMoid root, vector<VIDX_PAIR>& vertexIdxMata, vector<idle_blk*> &update_blk_vec, graph_blk* gblk, size_v vertex_index)
{
	/*
		���洢���µĶ���洢������blk�� (gblk�еĵ�vertex_index��)
	*/
	size_v vid = gblk->buf[vertex_index * 3];
	size_v degree = gblk->buf[vertex_index * 3 + 1];
	size_v ori_offset = gblk->buf[vertex_index * 3 + 2];

	size_v blkIdx = findIdleBlk(update_blk_vec, degree);  // �˴���buffer��Ҫ���¸������һ��������buffer״̬
//	size_v blkIdx = findIdleBlkInsert(idle_blk_map, degree);
	if (blkIdx != -1)  // �ҵ����ʵ�blk
	{
		graph_blk* target_blk = graph_blk_vec[blkIdx];
		size_v start_idx = target_blk->vcnt * 3;  // ��������Ϊ����λ��
		size_v preoff = target_blk->buf[start_idx - 1];
		size_v predegree = target_blk->buf[start_idx - 2];

	//	size_v prebuf = ceil((float)predegree * BUF_PRO);  // �˴�buffer ����ȷ��
	//	size_v offset = preoff - predegree - prebuf;  // ǰһ������(���һ������)����ʼpreoff - degree - buffer���ȣ���Ϊ��ǰ������ʼoffset
		size_v offset = target_blk->next_pos;  // ��ʵ����ڽڵ��λ��

		target_blk->buf[start_idx] = vid;
		pmemobj_persist(pop, &(target_blk->buf[start_idx]), sizeof(size_v));
		target_blk->buf[start_idx + 1] = degree;
		pmemobj_persist(pop, &(target_blk->buf[start_idx + 1]), sizeof(size_v));
		target_blk->buf[start_idx + 2] = offset;  // �ڸ�blk��offset����ʼ�洢�ڽڵ�
		pmemobj_persist(pop, &(target_blk->buf[start_idx + 2]), sizeof(size_v));

		for (size_v neiIdx = 0; neiIdx < degree; neiIdx++)
		{
			target_blk->buf[offset - neiIdx] = gblk->buf[ori_offset - neiIdx];
			pmemobj_persist(pop, &(target_blk->buf[offset - neiIdx]), sizeof(size_v));
		}

		target_blk->vcnt++;
		pmemobj_persist(pop, &(target_blk->vcnt), sizeof(size_v));

		size_v tmp_cap = degree + ceil((float)BUF_PRO * degree);
		target_blk->next_pos -= tmp_cap;  // �������Ҫpersistent

		update_vertex_index(vertexIdxMata, target_blk, vid);  // ���¶�������

		
		//print_array(target_blk->buf, MAX_LEN);
	}
	else  // �½�һ���µ�blk
	{
		graph_blk* target_blk = generate_new_blk(root);

		target_blk->buf[0] = vid;
		pmemobj_persist(pop, &(target_blk->buf[0]), sizeof(size_v));
		target_blk->buf[1] = degree;
		pmemobj_persist(pop, &(target_blk->buf[1]), sizeof(size_v));

		size_v offset = MAX_LEN - 1;
		target_blk->buf[2] = offset;  //  ��ʼ�洢λ��
		pmemobj_persist(pop, &(target_blk->buf[2]), sizeof(size_v));

		for (size_v neiIdx = 0; neiIdx < degree; neiIdx++)
		{
			target_blk->buf[offset - neiIdx] = gblk->buf[ori_offset - neiIdx];
			pmemobj_persist(pop, &(target_blk->buf[offset - neiIdx]), sizeof(size_v));
		}

		target_blk->vcnt++;
		pmemobj_persist(pop, &(target_blk->vcnt), sizeof(size_v));
		

		size_v cap = ceil((float)degree * BUF_PRO) + degree + 3;
		size_v capmod = MAX_LEN - cap;

		target_blk->next_pos = MAX_LEN - 1 - (cap - 3);

		//idle_blk_map[target_blk->blk_id] = capmod;

		idle_blk* iblk = (idle_blk*)malloc(sizeof(idle_blk));  // in DRAM

		iblk->blk_idx = target_blk->blk_id;
		iblk->max_cap = capmod; // ����Ϊ����
		iblk->next_pos = MAX_LEN - 1 - (cap - 3);  //  ��һ���ɴ洢�ڽڵ��λ��
		update_blk_vec.push_back(iblk);

		//print_array(target_blk->buf, MAX_LEN);
		//cout << target_blk->next_pos << endl;

		update_vertex_index(vertexIdxMata, target_blk, vid);  // ���¶�������
	}
}

void update_blk(PMEMobjpool* pop, graph_blk* gblk, size_v max_vidx, size_v buffer_cnt)
{
	/*
		����ǰmax_vidx���������buffer�ط���
		�����һ�����㿪ʼ�ƶ�
		�ƶ���ɺ���µ�ǰblk�洢�Ķ�����
	*/
	gblk->vcnt = max_vidx + 1;
	pmemobj_persist(pop, &(gblk->vcnt), sizeof(size_v));

	for (size_v vidx = max_vidx; vidx >= 0; vidx--)
	{
		size_v degree = gblk->buf[vidx * 3 + 1];
		size_v offset = gblk->buf[vidx * 3 + 2];
		size_v lastNeiPos = offset - degree + 1;  // ���һ���ڽڵ�洢��λ��
		size_v tmpBufSize = ceil((float)degree * BUF_PRO);  // ��ǰ��������Ҫ��buffer size
		buffer_cnt -= tmpBufSize;

		size_v new_offset = offset - buffer_cnt;  // �ճ�buffer��ʣ�ඥ�� �����һ��������ڽڵ���������buffer_cnt��slot��
		// ��һ����Ҫȫ���ƶ�����Ϊ�ڽڵ��������ܴ���ʣ��buffer���ȣ������ֻ���ƶ������ڽڵ�
		if (new_offset > lastNeiPos)
		{
			for (size_v neiIdx = 0; neiIdx < buffer_cnt; neiIdx++)
			{
				gblk->buf[lastNeiPos - 1 - neiIdx] = gblk->buf[offset - neiIdx];  // ֻ��Ҫ��ǰ�غϲ����ƶ�ֻbuffer�����ɣ��������ݸ���
				pmemobj_persist(pop, &(gblk->buf[lastNeiPos - 1 - neiIdx]), sizeof(size_v));
			}
		}
		else   // �����ڽڵ㶼��Ҫ�ƶ�
		{
			for (size_v neiIdx = 0; neiIdx < degree; neiIdx++)
			{
				gblk->buf[new_offset - neiIdx] = gblk->buf[offset - neiIdx];
				pmemobj_persist(pop, &(gblk->buf[new_offset - neiIdx]), sizeof(size_v));
			}
		}
		gblk->buf[vidx * 3 + 2] = new_offset;  // �����ڽڵ��ƶ���� ����offset
		pmemobj_persist(pop, &(gblk->buf[vidx * 3 + 2]), sizeof(size_v));

		if (vidx == max_vidx)
		{
			gblk->next_pos = new_offset - degree - tmpBufSize;  // ���һ������ȷ��λ�ú󼴿�ȷ����ǰblk��next_pos
		}
	}

	//print_array(gblk->buf, MAX_LEN);
/*	gblk->vcnt = max_vidx + 1;
	pmemobj_persist(pop, &(gblk->vcnt), sizeof(size_v));*/   // �ƶ���ɺ���µ�ǰblk�洢�Ķ�����
}

void generate_and_split(PMEMobjpool* pop, PMEMoid root, vector<VIDX_PAIR>& vertexIdxMata, vector<idle_blk*>& update_blk_vec, graph_blk* gblk, size_v src, size_v des)
{
	/*
		step 0: ȷ����ǰgblk�ܴ洢����ඥ����
		step 1: ������������䵽�����п��е�blk��
		step 2: �޿���blk�������µ�blk�����������������blk��
	*/
	size_v total_occupation = 0;
	size_v buffer_cnt = 0;
	size_v max_vidx = 0;
	for (size_v vidx = 0; vidx < gblk->vcnt; vidx++)
	{
		size_v degree = gblk->buf[vidx * 3 + 1];
		size_v occupation = 3 + degree + ceil((float)degree * BUF_PRO);
		total_occupation += occupation;
		if (total_occupation > MAX_LEN)
		{
			max_vidx = vidx - 1;  // ֻ�����㵽ǰһ������Ĵ洢����
			break;
		}
		buffer_cnt += ceil((float)degree * BUF_PRO);
	}

	//cout << "maximum occupation: " << max_vidx << endl;

	for (size_v vidx = max_vidx + 1; vidx < gblk->vcnt; vidx++)
	{
		remalloc_vertex(pop, root, vertexIdxMata, update_blk_vec, gblk, vidx);
	}

	//cout << "remalloc_vertex complete !!!" << endl;

	// �����ж������·�����ɺ󣬸���ԭgblk�еĶ������������·������slot(�������),����0-max_vidx�Ķ������buffer�ط��䣨����BUF_PRO��
	update_blk(pop, gblk, max_vidx, buffer_cnt);

	//print_array(gblk->buf, MAX_LEN);
	//cout << "update_blk complete !!!" << endl;

	// ֱ�Ӳ������ڽڵ㣬��ʱ�Լ�buffer���㣬ֱ�Ӳ��뼴��
	VIDX_PAIR srcp = vertexIdxMata[src];
	size_v blk_id = srcp.first;
	size_v v_offset = srcp.second;  // �洢�ĵڼ�������
	graph_blk* gblk_new = graph_blk_vec[blk_id];
	size_v vidx = v_offset * 3;
	size_v degree = gblk_new->buf[vidx + 1];
	size_v nei_offset = gblk_new->buf[vidx + 2];

	gblk_new->buf[nei_offset - degree] = des;
	pmemobj_persist(pop, &(gblk_new->buf[nei_offset - degree]), sizeof(size_v));
	gblk->buf[vidx + 1] += 1;  // degree��1
	pmemobj_persist(pop, &(gblk->buf[vidx + 1]), sizeof(size_v));

}

void super_vertex_insertion(PMEMobjpool* pop, PMEMoid root, graph_blk* gblk, size_v value)
{
	/*
	* gblk���洢��Ҫ�������ڽڵ�Ķ������ڵ�blk
	* value���������ֵ
		1. ���ݵ�ǰblk���洢���ڽڵ������ж��Ƿ�ӵ�ǰblk�������ڽڵ�
		2. ��ǰblk�����ˣ�����next_blk����һblk��Ѱ�ҿ���slot
		3. ���ҵ����һ��blk����next_blkΪ-1����Ҳ���������½�һ����blk��������ǰblk��next_blk�޸�Ϊ��Ӧ��blk��
	*/
	size_v next_blk;
	size_v degree;
	graph_blk* cur_blk = gblk;

	while (cur_blk->next_blk)
	{
		degree = cur_blk->buf[1];
		if (degree == MAX_LEN - 3)  // ������
		{
			next_blk = cur_blk->next_blk;
			cur_blk = graph_blk_vec[next_blk];
		}
		else
		{
			break;  // ��ǰcur_blk���ڿ���buffer
		}
	}

	if (cur_blk->next_blk == -1)  // ���һ��blk����һ���ж��Ƿ��п���slot
	{
		degree = cur_blk->buf[1];
		if (degree == MAX_LEN - 3)   // �����ˣ���Ҫ�½��µ�blk
		{
			graph_blk* new_blk = generate_new_blk(root);
			new_blk->buf[0] = cur_blk->buf[0];
			new_blk->buf[1] = 1;
			new_blk->buf[2] = MAX_LEN - 1;  // ĩλ
			new_blk->buf[MAX_LEN - 1] = value;
			pmemobj_persist(pop, new_blk, sizeof(graph_blk));
			
			cur_blk->next_blk = new_blk->blk_id;
			pmemobj_persist(pop, &(cur_blk->next_blk), sizeof(size_v));
		}
		else
		{
			size_v offset = cur_blk->buf[2];
			cur_blk->buf[offset - degree] = value;
			pmemobj_persist(pop, &(cur_blk->buf[offset - degree]), sizeof(size_v));
			cur_blk->buf[1] += 1;
			pmemobj_persist(pop, &(cur_blk->buf[1]), sizeof(size_v));
		}
	}
	else   // ��Ȼ�п���slot
	{
		size_v offset = cur_blk->buf[2];
		cur_blk->buf[offset - degree] = value;
		pmemobj_persist(pop, &(cur_blk->buf[offset - degree]), sizeof(size_v));
		cur_blk->buf[1] += 1;
		pmemobj_persist(pop, &(cur_blk->buf[1]), sizeof(size_v));
	}
}

void insert_edge(PMEMobjpool* pop, PMEMoid root, vector<VIDX_PAIR>& vertexIdxMata, vector<idle_blk*>& update_blk_vec, size_v src, size_v des)
{
	/*
		* pop���ڴ�أ����ڳ־ô洢����
		* root�������½�blk
		* vertexIdxMata�����ݶ���id��ѯ��������blk��ƫ����v_offset
		* update_blk_vec: ���������������blk����
		* src/des: ��des����src���ڽڵ��б���
	
		* insert des into src
			case 1: enough buffer
			case 2: borrow from neighbors (maybe n-hop neighbors)
			case 3: split block (re-malloc buffer according to BUF_PRO)
			case 4: ��Ե������޷��洢�ڵ���blk����� ������ɣ�
	*/
	VIDX_PAIR srcp = vertexIdxMata[src];
	size_v blk_id = srcp.first;
	size_v v_offset = srcp.second;  // �洢�ĵڼ�������
	graph_blk* gblk = graph_blk_vec[blk_id];
	size_v vidx = v_offset * 3;
	size_v degree = gblk->buf[vidx + 1];
	size_v nei_offset = gblk->buf[vidx + 2];
	size_v buffer_size = 0;
	size_v next_offset = gblk->buf[vidx + 5];

	// �ж��Ƿ��ǳ������㣨���blk�洢��
	if (gblk->next_blk != -1)
	{
		cout << "case 0: super vertex!" << endl;

		super_vertex_insertion(pop, root, gblk, des);
		return;
	}

	if (v_offset + 1 < gblk->vcnt) // �������һ������
	{
		buffer_size = nei_offset - next_offset - degree;
	}
	else
	{
	//	buffer_size = nei_offset - degree - (v_offset + 2);  // ���һ��������������ڽڵ�
		buffer_size = nei_offset - degree - gblk->next_pos;  //  ���һ�������buffer
	}

	if (buffer_size > 0)  // case 1
	{
		cout << "case 1: have enough buffer slots!" << endl;

		size_v insert_pos = nei_offset - degree;  

		gblk->buf[insert_pos] = des;
		pmemobj_persist(pop, &(gblk->buf[insert_pos]), sizeof(size_v));

		gblk->buf[vidx + 1] += 1;  // degree��1
		pmemobj_persist(pop, &(gblk->buf[vidx + 1]), sizeof(size_v));
	}
	else  // buffer������
	{
		// case 2: borrow from neighbors (maybe n-hop neighbors)

		size_v owner_pos = detect_buffer(gblk, v_offset);
		cout << "owner_pos: " << owner_pos << endl;

		if (owner_pos != -1)  // �ҵ�����slot
		{
			cout << "case 2: borrow slots from neighbors!" << endl;

			exchange_slot(pop, gblk, owner_pos, v_offset, des);
		}
		else  // case 3: require new blk -> split the original blk
		{
			cout << "case 3: generate new block!" << endl;

			generate_and_split(pop, root, vertexIdxMata, update_blk_vec, gblk, src, des);
		}
	}

}

void delete_edge(PMEMobjpool* pop, size_v src, size_v des, vector<VIDX_PAIR> vertexIdxMata)
{
	/*
		delete des from src
		��Ҫɾ�����ڽڵ������һ���ڽڵ����λ�ã����޸�offset
		��ע������Դ��������㣩
	*/
	VIDX_PAIR srcp = vertexIdxMata[src];
	size_v blk_id = srcp.first;
	size_v v_offset = srcp.second;  // �洢�ĵڼ�������
	graph_blk* gblk = graph_blk_vec[blk_id];
	size_v vidx = v_offset * 3;
	size_v degree = gblk->buf[vidx + 1];
	size_v nei_offset = gblk->buf[vidx + 2];

	// ���������㣺�ڱ�ɾ���ڽڵ�����blk����ɾ�����ɣ�����ĵ�ǰblk��degreeֵ
	if (gblk->next_blk != -1)
	{
		graph_blk* cur_blk = gblk;
		degree = cur_blk->buf[1];
		nei_offset = cur_blk->buf[2];
		size_v break_flag = 0;
		size_v next_blk = cur_blk->next_blk;
		while (break_flag == 0)
		{
			for (size_v neiIdx = 0; neiIdx < degree; neiIdx++)
			{
				if (cur_blk->buf[nei_offset - neiIdx] == des)
				{
					cur_blk->buf[nei_offset - neiIdx] = cur_blk->buf[nei_offset - degree + 1]; // ��ĩβ�ڽڵ㸴������Ҫɾ�����ڽڵ㴦
					pmemobj_persist(pop, &(cur_blk->buf[nei_offset - neiIdx]), sizeof(size_v));
					cur_blk->buf[vidx + 1] -= 1;  // ��ǰblk degree��1
					pmemobj_persist(pop, &(cur_blk->buf[vidx + 1]), sizeof(size_v));  // recovery��ʱ����Ҫ�����ж��Ƿ���������һ���ڽڵ�id��ͬ���ڽڵ㣬�������˵���˴�flushʱcrash��
					
					break_flag = 1;
					break;
				}
			}

			if (break_flag == 0)
			{
				cur_blk = graph_blk_vec[next_blk];
				degree = cur_blk->buf[1];
				nei_offset = cur_blk->buf[2];
				next_blk = cur_blk->next_blk;
			}
		}

		return;
	}


	// ������ͨ���㣬������blk�洢�˶������
	for (size_v neiIdx = 0; neiIdx < degree; neiIdx++)
	{
		if (gblk->buf[nei_offset - neiIdx] == des)
		{
			gblk->buf[nei_offset - neiIdx] = gblk->buf[nei_offset - degree + 1]; // ��ĩβ�ڽڵ㸴������Ҫɾ�����ڽڵ㴦
			pmemobj_persist(pop, &(gblk->buf[nei_offset - neiIdx]), sizeof(size_v));
			gblk->buf[vidx + 1] -= 1;
			pmemobj_persist(pop, &(gblk->buf[vidx + 1]), sizeof(size_v));  // recovery��ʱ����Ҫ�����ж��Ƿ���������һ���ڽڵ�id��ͬ���ڽڵ㣬�������˵���˴�flushʱcrash��
			return;
		}
	}
}

void insert_single_edge(PMEMobjpool* pop, PMEMoid root, vector<VIDX_PAIR>& vertexIdxMata, vector<idle_blk*>& update_blk_vec, newEdge edge)
{
	/*
		case 1: enough buffer
		case 2: borrow from neighbors
		case 3: split block (re-malloc buffer according to BUF_PRO)
	*/
	size_v srcv = edge.src;
	size_v desv = edge.des;

	insert_edge(pop, root, vertexIdxMata, update_blk_vec, srcv, desv);
}

void insert_batch_edges(void)
{

}

void delete_single_edge(PMEMobjpool* pop, vector<VIDX_PAIR> vertexIdxMata, newEdge edge)
{
	size_v srcv = edge.src;
	size_v desv = edge.des;

	delete_edge(pop, srcv, desv, vertexIdxMata);
}

void graph_recovery(void)
{
	
}

void kcore_recovery(void)
{

}




#endif
