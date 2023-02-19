// AMRTREEVIEW.JAVA
// MIT License
// Copyright (c) 2022 AmrDeveloper (Amr Hesham)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// Adopted for Stylo-Q by A.Sobolev 2023
//
package ru.petroglif.styloq;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.LinkedList;
import java.util.List;

public class AmrTreeView {
	private static final int ViewTypeOrdinary = 0;
	private static final int ViewTypeFolder   = 1;
	private static final int _ViewTypeCount   = 2;
	//
	// TreeNode is a container for the value to represent a node on the TreeView
	//
	public static class TreeNode {
		private static final int statusExpanded = 0x0001;
		private static final int statusSelected = 0x0002;
		private Object value;
		private TreeNode parent;
		private LinkedList<TreeNode> children;
		private int OriginalIndex; // Индекс элемента в прототипе списка. Нужен для
		private int layoutId;
		private int level;
		private int Status;
		//private boolean isExpanded;
		//private boolean isSelected;

		public TreeNode(Object value, int originalIndex, int layoutId)
		{
			this.value = value;
			this.parent = null;
			this.children = new LinkedList<>();
			this.layoutId = layoutId;
			this.level = 0;
			Status = 0;
			OriginalIndex = originalIndex;
			//this.isExpanded = false;
			//this.isSelected = false;
		}
		public void addChild(TreeNode child)
		{
			child.setParent(this);
			child.setLevel(level + 1);
			children.add(child);
			updateNodeChildrenDepth(child);
		}
		public void setValue(Object value) { this.value = value; }
		public Object getValue() { return value; }
		public void setParent(TreeNode parent) { this.parent = parent; }
		public TreeNode getParent() { return parent; }
		public TreeNode GetTopmostParent()
		{
			TreeNode result = parent;
			while(result != null) {
				if(result.parent != null)
					result = result.parent;
				else
					break;
			}
			return result;
		}
		public LinkedList<TreeNode> getChildren() { return children; }
		public int getLayoutId() { return layoutId; }
		public void setLevel(int level) { this.level = level; }
		public int getLevel() { return level; }
		public void setExpanded(boolean expanded)
		{
			if(expanded)
				Status |= statusExpanded;
			else
				Status &= ~statusExpanded;
			//isExpanded = expanded;
		}
		public boolean isExpanded() { return (Status & statusExpanded) != 0 ? true : false/*isExpanded*/; }
		public void setSelected(boolean selected)
		{
			if(selected)
				Status |= statusSelected;
			else
				Status &= ~statusSelected;
			//isSelected = selected;
		}
		public boolean isSelected() { return (Status & statusSelected) != 0 ? true : false/*isSelected*/; }
		private void updateNodeChildrenDepth(TreeNode node)
		{
			if(!node.getChildren().isEmpty()) {
				for(TreeNode child : node.getChildren()) {
					child.setLevel(node.getLevel() + 1);
				}
			}
		}
		//
		// Descr: Ищет элемент с оригинальным индексом равным idx.
		//   При поиске просматривается толко this-объект и его потомки, на родитескую ветку (parent)
		//   поиск не распространяется.
		//
		public TreeNode FindOriginalIndex(int idx)
		{
			TreeNode result = null;
			if(idx >= 0) {
				if(OriginalIndex == idx)
					result = this;
				else if(children != null) {
					for(TreeNode child : children) {
						if(child != null) {
							result = child.FindOriginalIndex(idx); // @recursion
							if(result != null)
								break;
						}
					}
				}
			}
			return result;
		}
	}
	//
	// Manager class for TreeNodes to easily apply operations on them
	// and to make it easy for testing and extending
	//
	public static class TreeNodeManager {
		private final LinkedList<TreeNode> RootsNodes; // Collection to save the current tree nodes
		public TreeNodeManager()
		{
			RootsNodes = new LinkedList<>();
		}
		public TreeNode FindOriginalIndex(int idx)
		{
			TreeNode result = null;
			if(idx >= 0) {
				for(TreeNode item : RootsNodes) {
					if(item != null) {
						result = item.FindOriginalIndex(idx);
						if(result != null)
							break;
					}
				}
			}
			return result;
		}
		//
		// Set the current visible tree nodes
		// @param treeNodes New tree nodes
		//
		public void setTreeNodes(List<TreeNode> treeNodes)
		{
			RootsNodes.clear();
			RootsNodes.addAll(treeNodes);
		}
		//
		// Get the Current visible Tree nodes
		// @return The visible Tree nodes main
		//
		public List<TreeNode> getTreeNodes() { return RootsNodes; }
		//
		// Get TreeNode from the current nodes by index
		// @param index of node to get it
		// @return TreeNode from by index from current tree nodes if exists
		//
		public TreeNode get(int index) { return RootsNodes.get(index); }
		//
		// Add new node to the current tree nodes
		// @param node to add it to the current tree nodes
		// @return true of this node is added
		//
		public boolean addNode(TreeNode node) { return RootsNodes.add(node); }
		//
		// Clear the current nodes and insert new nodes
		// @param newNodes to update the current nodes with them
		//
		public void updateNodes(List<TreeNode> newNodes)
		{
			RootsNodes.clear();
			RootsNodes.addAll(newNodes);
		}
		//
		// Delete one node from the visible nodes
		// @param node to delete it from the current nodes
		// @return true of this node is deleted
		//
		public boolean removeNode(TreeNode node) { return RootsNodes.remove(node); }
		//
		// Clear the current nodes
		//
		public void clearNodes() { RootsNodes.clear(); }
		//
		// Get the current number of visible nodes
		// @return the size of visible nodes
		//
		public int size() { return RootsNodes.size(); }
		//
		// Collapsing node and all of his children
		// @param node The node to collapse it
		// @return the index of this node if it exists in the list
		//
		public int collapseNode(TreeNode node)
		{
			int position = RootsNodes.indexOf(node);
			if(position != -1 && node.isExpanded()) {
				node.setExpanded(false);
				LinkedList<TreeNode> deletedParents = new LinkedList<>(node.getChildren());
				RootsNodes.removeAll(node.getChildren());
				for(int i = position + 1; i < RootsNodes.size(); i++) {
					TreeNode iNode = RootsNodes.get(i);
					if(deletedParents.contains(iNode.getParent())) {
						deletedParents.add(iNode);
						deletedParents.addAll(iNode.getChildren());
					}
				}
				RootsNodes.removeAll(deletedParents);
			}
			return position;
		}
		//
		// Expanding node and all of his children
		// @param node The node to expand it
		// @return the index of this node if it exists in the list
		//
		public int expandNode(TreeNode node)
		{
			int position = RootsNodes.indexOf(node);
			if(position != -1 && !node.isExpanded()) {
				node.setExpanded(true);
				RootsNodes.addAll(position + 1, node.getChildren());
				for(TreeNode child : node.getChildren()) {
					if(child.isExpanded())
						updateExpandedNodeChildren(child);
				}
			}
			return position;
		}
		//
		// Update the list for expanded node
		// to expand any child of his children that is already expanded before
		// @param node that just expanded now
		//
		private void updateExpandedNodeChildren(TreeNode node)
		{
			int position = RootsNodes.indexOf(node);
			if(position != -1 && node.isExpanded()) {
				RootsNodes.addAll(position + 1, node.getChildren());
				for(TreeNode child : node.getChildren()) {
					if(child.isExpanded())
						updateExpandedNodeChildren(child);
				}
			}
		}
		//
		// @param  node The node to collapse the branch of it
		// @return the index of this node if it exists in the list
		//
		public int collapseNodeBranch(TreeNode node)
		{
			int position = RootsNodes.indexOf(node);
			if(position != -1 && node.isExpanded()) {
				node.setExpanded(false);
				for(TreeNode child : node.getChildren()) {
					if(!child.getChildren().isEmpty())
						collapseNodeBranch(child);
					RootsNodes.remove(child);
				}
			}
			return position;
		}
		//
		// Expanding node full branches
		// @param  node The node to expand the branch of it
		// @return the index of this node if it exists in the list
		//
		public int expandNodeBranch(TreeNode node)
		{
			int position = RootsNodes.indexOf(node);
			if(position != -1 && !node.isExpanded()) {
				node.setExpanded(true);
				int index = position + 1;
				for(TreeNode child : node.getChildren()) {
					int before = RootsNodes.size();
					RootsNodes.add(index, child);
					expandNodeBranch(child);
					int after = RootsNodes.size();
					int diff = after - before;
					index += diff;
				}
			}
			return position;
		}
		//
		// Expanding one node branch to until specific level
		// @param node to expand branch of it until level
		// @param level to expand node branches to it
		//
		public void expandNodeToLevel(TreeNode node, int level)
		{
			if(node.getLevel() <= level)
				expandNode(node);
			for(TreeNode child : node.getChildren()) {
				expandNodeToLevel(child, level);
			}
		}
		//
		// Expanding all tree nodes branches to until specific level
		// @param level to expand all nodes branches to it
		//
		public void expandNodesAtLevel(int level)
		{
			for(int i = 0; i < RootsNodes.size() ; i++) {
				TreeNode node = RootsNodes.get(i);
				expandNodeToLevel(node, level);
			}
		}
		//
		// Collapsing all nodes in the tree with their children
		//
		public void collapseAll()
		{
			List<TreeNode> treeNodes = new LinkedList<>();
			for(int i = 0; i < RootsNodes.size(); i++) {
				TreeNode root = RootsNodes.get(i);
				if(root.getLevel() == 0) {
					collapseNodeBranch(root);
					treeNodes.add(root);
				}
				else {
					root.setExpanded(false);
				}
			}
			updateNodes(treeNodes);
		}
		//
		// Expanding all nodes in the tree with their children
		//
		public void expandAll()
		{
			for(int i = 0; i < RootsNodes.size(); i++) {
				TreeNode root = RootsNodes.get(i);
				expandNodeBranch(root);
			}
		}
	}
	public static class ClickListener implements View.OnClickListener, View.OnLongClickListener {
		private int Index;
		private final LayoutInflater Inflater;
		private Adapter Adapter;
		private Holder Holder;
		public ClickListener(LayoutInflater inflater, Adapter adapter, Holder holder, int idx)
		{
			Inflater = inflater;
			Adapter = adapter;
			Holder = holder;
			Index = idx;
		}
		@Override public void onClick(View v)
		{
			SLib.EventHandler eh = Adapter.GetEventHandler(Inflater.getContext());
			if(eh != null) {
				SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
				ev_subj.RvHolder = Holder;
				ev_subj.ItemIdx = Index;
				ev_subj.ItemView = v;
				eh.HandleEvent(SLib.EV_LISTVIEWITEMCLK, Adapter, ev_subj);
			}
		}
		@Override public boolean onLongClick(View v)
		{
			boolean result = false;
			SLib.EventHandler eh = Adapter.GetEventHandler(Inflater.getContext());
			if(eh != null) {
				SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
				ev_subj.RvHolder = Holder;
				ev_subj.ItemIdx = Index;
				ev_subj.ItemView = v;
				eh.HandleEvent(SLib.EV_LISTVIEWITEMLONGCLK, null, ev_subj);
				result = true;
			}
			return result;
		}
	}
	//
	// Custom RecyclerView.Adapter used to provide a tree view features on any RecyclerView
	//
	public static class Adapter extends RecyclerView.Adapter <Holder> {
		//
		// Interface definition for a callback to be invoked when a TreeNode has been clicked and held.
		//
		public interface OnTreeNodeClickListener
		{
			//
			// Called when a TreeNode has been clicked.
			// @param treeNode The current clicked node
			// @param view The view that was clicked and held.
			//
			void onTreeNodeClick(TreeNode treeNode, View view);
		}
		//
		// Interface definition for a callback to be invoked when a TreeNode has been clicked and held.
		//
		public interface OnTreeNodeLongClickListener
		{
			//
			// Called when a TreeNode has been clicked and held.
			// @param treeNode The current clicked node
			// @param view The view that was clicked and held.
			// @return true if the callback consumed the long click, false otherwise.
			//
			boolean onTreeNodeLongClick(TreeNode treeNode, View view);
		}
		//
		private int ListRcId;
		private int FocusedIdx; // Если (>= 0 && < getCount()), то управляющий класс может отобразать специальным образом. It's original index!
		private int FoldingButtonRcID; // Ид кнопки, управляющей сворачиванием/разворачиванием веток дерева
			// Если 0, то нажание в любой точке элемента-папки приведет к сворачиванию/разворачиванию.
		private LinearLayout _Lo;
		private final LayoutInflater Inflater;
		private SLib.EventHandler EventReceiver; // Если !null, то методы класса проверяют, чтобы объект был
		// адекватным получателем сообщений (SlActivity, SlDialog etc) и отправляет сообщения ему.
		// Если null, то сообщения отправляются в контекст (если он является SlActivity)
		//
		// Manager class for TreeNodes to easily apply operations on them
		// and to make it easy for testing and extending
		//
		private final TreeNodeManager Mgr;
		//private final HolderFactory treeViewHolderFactory; // A ViewHolder Factory to get TreeViewHolder object that mapped with layout
		private TreeNode currentSelectedNode; // The current selected Tree Node
		//private Adapter.OnTreeNodeClickListener treeNodeClickListener; // Custom OnClickListener to be invoked when a TreeNode has been clicked.
		//private Adapter.OnTreeNodeLongClickListener treeNodeLongClickListener; // Custom OnLongClickListener to be invoked when a TreeNode has been clicked and hold.
		//
		// Simple constructor
		// @param factory a View Holder Factory mapped with layout id's
		//
		//public Adapter(HolderFactory factory)
		public Adapter(Context ctx, SLib.EventHandler eventReceiver, int listRcId)
		{
			EventReceiver = eventReceiver;
			ListRcId = listRcId;
			FocusedIdx = -1;
			Inflater = LayoutInflater.from(ctx);
			//this.treeViewHolderFactory = factory;
			this.Mgr = new TreeNodeManager();
		}
		public void SetFoldingButtonId(int rcId) { FoldingButtonRcID = rcId; }
		public int  GetFoldingButtonId() { return FoldingButtonRcID; }
		public int  GetListRcId() { return ListRcId; }
		public void SetFocusedIndex(int idx) // FocusedIdx - is original index (not an inner)
		{
			FocusedIdx = idx;
			if(Mgr != null) {
				TreeNode focused_item = Mgr.FindOriginalIndex(FocusedIdx);
				if(focused_item != null) {
					TreeNode topmost_parent = focused_item.GetTopmostParent();
					if(topmost_parent != null)
						expandNode(topmost_parent);
				}
			}
		}
		public int  GetFocusedIndex() { return FocusedIdx; }
		//
		// Constructor used to accept user custom TreeNodeManager class
		// @param factory a View Holder Factory mapped with layout id's
		// @param manager a custom tree node manager class
		//
		@NonNull @Override public Holder onCreateViewHolder(@NonNull ViewGroup parent, int itemRcId)
		{
			View view = LayoutInflater.from(parent.getContext()).inflate(itemRcId, parent, false);
			//return treeViewHolderFactory.getTreeViewHolder(view, layoutId);
			Holder holder = null;
			//View view = null;
			Context ctx = Inflater.getContext();
			if(ctx != null) {
				SLib.EventHandler eh = GetEventHandler(ctx);
				if(itemRcId > 0) {
					View root_view = parent;//parent.getRootView();
					if(root_view != null && root_view instanceof ViewGroup) {
						view = Inflater.inflate(itemRcId, (ViewGroup)root_view, false);
						holder = new Holder(view, this);
						if(eh != null) {
							SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
							ev_subj.RvHolder = holder;
							ev_subj.ItemView = view;
							ev_subj.ItemIdx = -1; //itemRcId;
							Object ev_result = eh.HandleEvent(SLib.EV_CREATEVIEWHOLDER, this, ev_subj);
						}
					}
				}
				else if(eh != null) {
					SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
					ev_subj.RvHolder = null;
					ev_subj.ItemView = parent;
					ev_subj.ItemIdx = -1; //itemRcId;
					Object ev_result = eh.HandleEvent(SLib.EV_CREATEVIEWHOLDER, this, ev_subj);
					holder = (ev_result != null && ev_result instanceof Holder) ? (Holder)ev_result : null;
				}
			}
			return holder;
		}
		public SLib.EventHandler GetEventHandler(Context ctx)
		{
			return (EventReceiver != null) ? EventReceiver : ((ctx instanceof SLib.SlActivity) ? (SLib.SlActivity)ctx : null);
		}
		private void OnFoldingClick(View v, TreeNode currentNode)
		{
			// Handle node selection
			currentNode.setSelected(true);
			if(currentSelectedNode != null)
				currentSelectedNode.setSelected(false);
			currentSelectedNode = currentNode;
			// Handle node expand and collapse event
			if(!currentNode.getChildren().isEmpty()) {
				boolean is_node_expanded = currentNode.isExpanded();
				if(is_node_expanded)
					collapseNode(currentNode);
				else
					expandNode(currentNode);
				currentNode.setExpanded(!is_node_expanded);
			}
			notifyDataSetChanged();
			// Handle TreeNode click listener event
			//if(treeNodeClickListener != null)
			//	treeNodeClickListener.onTreeNodeClick(currentNode, v);
		}
		@Override public void onBindViewHolder(@NonNull Holder holder, int position)
		{
			final int idx = holder.getAdapterPosition();
			if(idx >= 0 && idx < Mgr.size()) {
				TreeNode current_node = Mgr.get(idx);
				holder.bindTreeNode(current_node);
				Context ctx = (_Lo != null) ? _Lo.getContext() : Inflater.getContext();
				boolean folding_clk_settled = false;
				if(ctx != null) {
					SLib.EventHandler eh = GetEventHandler(ctx);
					if(eh != null) {
						SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
						ev_subj.RvHolder = holder;
						ev_subj.ItemIdx = (current_node.OriginalIndex >= 0) ? current_node.OriginalIndex : idx;
						ev_subj.ItemObj = current_node.value;
						if(current_node.getChildren() != null) {
							ev_subj.IsFolder = true;
							ev_subj.IsCollapsedFolder = !current_node.isExpanded();
						}
						eh.HandleEvent(SLib.EV_GETLISTITEMVIEW, this, ev_subj);
						{
							ClickListener listener = new ClickListener(Inflater, this, holder, current_node.OriginalIndex);
							holder.itemView.setOnClickListener(listener);
							holder.itemView.setOnLongClickListener(listener);
						}
						{
							View v = (FoldingButtonRcID != 0) ? holder.itemView.findViewById(FoldingButtonRcID) : null;
							if(v != null) {
								v.setOnClickListener(new View.OnClickListener()
								{
									@Override public void onClick(View v)
									{
										//eh.HandleEvent(SLib.EV_COMMAND, v, new Integer(hour));
										OnFoldingClick(v, current_node);
									}
								});
								folding_clk_settled = true;
							}
						}
						folding_clk_settled = true; // @debug
					}
				}
				if(!folding_clk_settled) {
					holder.itemView.setOnClickListener(v -> { OnFoldingClick(v, current_node); });
					folding_clk_settled = true;
				}
				// Handle TreeNode long click listener event
				//holder.itemView.setOnLongClickListener(v -> {
				//	return (treeNodeLongClickListener != null) ? treeNodeLongClickListener.onTreeNodeLongClick(current_node, v) : true;
				//});
			}
		}
		@Override public int getItemViewType(int position) { return Mgr.get(position).getLayoutId(); }
		@Override public int getItemCount()
		{
			return Mgr.size();
		}
		//
		// Collapsing node and all of his children
		// @param node The node to collapse it
		//
		public void collapseNode(TreeNode node)
		{
			int position = Mgr.collapseNode(node);
			if(position != -1)
				notifyDataSetChanged();
		}
		//
		// Expanding node and all of his children
		// @param node The node to expand it
		//
		public void expandNode(TreeNode node)
		{
			int position = Mgr.expandNode(node);
			if(position != -1)
				notifyDataSetChanged();
		}
		//
		// Collapsing full node branches
		// @param node The node to collapse it
		//
		public void collapseNodeBranch(TreeNode node)
		{
			Mgr.collapseNodeBranch(node);
			notifyDataSetChanged();
		}
		//
		// Expanding node full branches
		// @param node The node to expand it
		//
		public void expandNodeBranch(TreeNode node)
		{
			Mgr.expandNodeBranch(node);
			notifyDataSetChanged();
		}
		//
		// Expanding one node branch to until specific level
		// @param node to expand branch of it until level
		// @param level to expand node branches to it
		//
		public void expandNodeToLevel(TreeNode node, int level)
		{
			Mgr.expandNodeToLevel(node, level);
			notifyDataSetChanged();
		}
		//
		// Expanding all tree nodes branches to until specific level
		// @param level to expand all nodes branches to it
		//
		public void expandNodesAtLevel(int level)
		{
			Mgr.expandNodesAtLevel(level);
			notifyDataSetChanged();
		}
		//
		// Collapsing all nodes in the tree with their children
		//
		public void collapseAll()
		{
			Mgr.collapseAll();
			notifyDataSetChanged();
		}
		//
		// Expanding all nodes in the tree with their children
		//
		public void expandAll()
		{
			Mgr.expandAll();
			notifyDataSetChanged();
		}
		//
		// Update the list of tree nodes
		// @param treeNodes The new tree nodes
		//
		public void updateTreeNodes(List<TreeNode> treeNodes)
		{
			Mgr.updateNodes(treeNodes);
			notifyDataSetChanged();
		}
		//
		// Delete all tree nodes
		//
		public void clearTreeNodes()
		{
			int size = Mgr.size();
			Mgr.clearNodes();
			notifyItemRangeRemoved(0, size);
		}
		//
		// Register a callback to be invoked when this TreeNode is clicked
		// @param listener The callback that will run
		//
		//public void setTreeNodeClickListener(Adapter.OnTreeNodeClickListener listener)
		//{
		//	this.treeNodeClickListener = listener;
		//}
		//
		// Register a callback to be invoked when this TreeNode is clicked and held
		// @param listener The callback that will run
		//
		//public void setTreeNodeLongClickListener(Adapter.OnTreeNodeLongClickListener listener)
		//{
		//	this.treeNodeLongClickListener = listener;
		//}
		//
		// Set the current visible tree nodes and notify adapter data
		// @param treeNodes New tree nodes
		//
		public void setTreeNodes(List<TreeNode> treeNodes)
		{
			Mgr.setTreeNodes(treeNodes);
			notifyDataSetChanged();
		}
		//
		// Get the Current visible Tree nodes
		// @return The visible Tree nodes main
		//
		public List<TreeNode> getTreeNodes() { return Mgr.getTreeNodes(); }
		//
		// @return The current selected TreeNode
		//
		public TreeNode getSelectedNode() { return currentSelectedNode; }
	}
	//
	// Default RecyclerView.ViewHolder for the TreeView the default behaviour is to manage the padding,
	// user should create custom one for each different layout and override bindTreeNode
	//
	public static class Holder extends RecyclerView.ViewHolder implements View.OnClickListener {
		//
		// The default padding value for the TreeNode item
		//
		private int nodePadding = 50;
		Adapter Adapter;

		public Holder(@NonNull View itemView, Adapter adapter)
		{
			super(itemView);
			Adapter = adapter;
		}
		//
		// Bind method that provide padding and bind TreeNode to the view list item
		// @param node the current TreeNode
		//
		public void bindTreeNode(TreeNode node)
		{
			int padding = node.getLevel() * nodePadding;
			itemView.setPadding(padding, itemView.getPaddingTop(), itemView.getPaddingRight(), itemView.getPaddingBottom());
		}
		//
		// Modify the current node padding value
		// @param padding the new padding value
		//
		public void setNodePadding(int padding) {
			this.nodePadding = padding;
		}
		//
		// Return the current TreeNode padding value
		// @return The current padding value
		//
		public int getNodePadding() {
			return nodePadding;
		}
		@Override public void onClick(View view)
		{
			SLib.SlActivity activity = null;
			View v = this.itemView;
			ViewParent iter_view = v.getParent();
			while(iter_view != null) {
				if(iter_view instanceof RecyclerView) {
					Context ctx = ((RecyclerView)iter_view).getContext();
					if(ctx != null && ctx instanceof SLib.SlActivity)
						activity = (SLib.SlActivity)ctx;
					break;
				}
				else
					iter_view = iter_view.getParent();
			}
			SLib.EventHandler eh = Adapter.GetEventHandler(activity);
			if(eh != null) {
				SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
				ev_subj.RvHolder = this;
				ev_subj.ItemIdx = this.getLayoutPosition();
				ev_subj.ItemView = view;
				eh.HandleEvent(SLib.EV_LISTVIEWITEMCLK, Adapter, ev_subj);
			}
		}
	}
}
